# ESP32 Project — Agent Context

## Hardware

- **Board:** [Waveshare ESP32-H2-Zero](https://www.waveshare.com/wiki/ESP32-H2-Zero)
- **SoC:** ESP32-H2 — native IEEE 802.15.4 radio (Zigbee / Thread), no Wi-Fi, no Bluetooth Classic
- **Flash:** 4 MB
- **Key GPIOs available:** see Waveshare pinout; BOOT button on GPIO9, LED on GPIO8

## Toolchain & Framework

- **Framework:** [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/) v5.0+
- **Build system:** CMake via `idf.py`
- **Zigbee stack:** `espressif/esp-zigbee-lib ~1.6.0` + `espressif/esp-zboss-lib ~1.6.0`
- **Component manifest:** `main/idf_component.yml` per project
- **Menuconfig flag required:** `ZB_ED_ROLE` must be defined for End Device role

## Repository Layout

```
esp32/
├── AGENTS.md                  ← this file
├── auto_watering/             ← active project (see below)
├── ha_on_off_light/           ← reference: working Zigbee HA on/off light
└── blink/                     ← reference: basic blink example
```

## Active Project: `auto_watering`

### Goal

Expose a complete auto-watering system to a Zigbee-based Home Automation (HA) coordinator
(e.g. Home Assistant with a Zigbee coordinator).

### Physical Devices

| # | Device | Interface | Notes |
|---|--------|-----------|-------|
| 1–4 | Capacitive soil moisture sensors | ADC | Analog 0–3.3 V output |
| 5 | Water pump | PWM (LEDC) | Speed/duty controlled via PWM |

### Zigbee Device Modelling

Each physical device is represented as a **separate Zigbee endpoint** on one node
(all endpoints share the same IEEE address / Zigbee node):

| Endpoint | Zigbee Device Type | ZCL Cluster(s) | Physical mapping |
|----------|--------------------|----------------|-----------------|
| 1 | `ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID` | `REL_HUMIDITY_MEASUREMENT` (0x0405) | Sensor 1 |
| 2 | `ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID` | `REL_HUMIDITY_MEASUREMENT` (0x0405) | Sensor 2 |
| 3 | `ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID` | `REL_HUMIDITY_MEASUREMENT` (0x0405) | Sensor 3 |
| 4 | `ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID` | `REL_HUMIDITY_MEASUREMENT` (0x0405) | Sensor 4 |
| 5 | `ESP_ZB_HA_ON_OFF_OUTPUT_DEVICE_ID` | `ON_OFF` + `LEVEL_CONTROL` | Pump (PWM duty) |

- Moisture sensors use the **Relative Humidity Measurement cluster** (`0x0405`) — same wire
  format as Soil Moisture (0x0408, not in SDK 1.6.0): `MeasuredValue` uint16, 0–10000 = 0–100%.
  Home Assistant recognises this cluster natively.
- The pump endpoint uses **Level Control cluster** (`0x0008`); `CurrentLevel` (0–254) maps
  linearly to PWM duty cycle (0–100%). `ON_OFF` cluster controls pump on/off.

### Coding Conventions

- Follow the patterns in `ha_on_off_light/HA_on_off_light/main/esp_zb_light.c`:
  - Stack init in a dedicated `esp_zb_task` FreeRTOS task (4096 stack, priority 5)
  - Signal handling in `esp_zb_app_signal_handler`
  - Attribute changes handled in `zb_action_handler` → `zb_attribute_handler`
- Use `ESP_RETURN_ON_FALSE` / `ESP_ERROR_CHECK` for error handling
- Log tag pattern: `static const char *TAG = "MODULE_NAME";`
- ADC readings: use `esp_adc/adc_oneshot.h` (IDF 5 API)
- PWM: use `driver/ledc.h` (`ledc_timer_config_t` + `ledc_channel_config_t`)
- Periodic sensor polling: `esp_timer` or a dedicated FreeRTOS task; report attribute
  changes via `esp_zb_zcl_set_attribute_val()`

### Key API Reference

```c
// Register multiple endpoints
esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
esp_zb_ep_list_add_ep(ep_list, cluster_list, endpoint_config);
esp_zb_device_register(ep_list);

// Report an attribute update to the coordinator
esp_zb_zcl_report_attr_cmd_t report = {
    .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
    .attributeID  = ESP_ZB_ZCL_ATTR_SOIL_MOISTURE_VALUE_ID,
    .cluster_id   = ESP_ZB_ZCL_CLUSTER_ID_SOIL_MOISTURE,
    .clusterRole  = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
    .srcEndpoint  = endpoint_num,
};
esp_zb_zcl_report_attr_cmd_req(&report);
```

### `idf_component.yml` dependencies

```yaml
dependencies:
  espressif/esp-zboss-lib: "~1.6.0"
  espressif/esp-zigbee-lib: "~1.6.0"
  idf:
    version: ">=5.0.0"
```

### Build & Flash

```bash
cd auto_watering
idf.py set-target esp32h2
idf.py menuconfig        # enable ZB_ED_ROLE under Zigbee
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
