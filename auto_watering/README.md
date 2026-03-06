# Auto Watering for ESP-H2-Zero-M

ESP-IDF project skeleton for an `ESP-H2-Zero-M` based auto-watering controller with:

- 4 capacitive soil moisture sensors on `GPIO1`-`GPIO4`
- 1 MOSFET-driven micro water pump output on `GPIO10`
- Zigbee-oriented resource modeling for sensors, pump, controller, and diagnostics

## GPIO Layout

- `GPIO0`: reserved for fast SPI functions
- `GPIO1`: `ADC1_CH1` soil sensor 0
- `GPIO2`: `ADC1_CH2` soil sensor 1
- `GPIO3`: `ADC1_CH3` soil sensor 2
- `GPIO4`: `ADC1_CH4` soil sensor 3
- `GPIO10`-`GPIO14`: PWM-capable outputs, with `GPIO10` used for the pump MOSFET gate by default

## Project Layout

- `main/app_main.c` main control loop and startup
- `main/board_config.c` centralized board pinout and defaults
- `main/soil_sensors.c` ADC sampling, filtering, moisture conversion, faulting
- `main/pump_control.c` safe pump GPIO control, runtime limit, lockout
- `main/watering_logic.c` automation and manual watering policy
- `main/zigbee_resources.c` Zigbee-facing resource snapshot model and sync point
- `main/Kconfig.projbuild` runtime configuration knobs

## Current Behavior

- Initializes NVS, pump GPIO, ADC channels, controller logic, and resource model
- Samples all four sensors periodically
- Converts raw ADC values into filtered moisture percentage using wet/dry calibration references
- Treats out-of-range or failed sensor reads as faults
- Prevents automatic watering whenever any sensor is faulted
- Starts automatic watering when the driest valid sensor is at or below the configured threshold
- Enforces maximum pump runtime and post-run lockout
- Keeps an in-memory Zigbee resource snapshot for:
  - 4 soil sensor endpoints starting at `0x10`
  - controller endpoint `0x01`
  - pump endpoint `0x20`
  - diagnostics endpoint `0x21`

## Build

Typical ESP-IDF flow:

```sh
idf.py set-target esp32h2
idf.py build
idf.py flash monitor
```

## Important Assumptions

- The pump MOSFET is active-high on `GPIO10`
- All four capacitive sensors can be read directly from `ADC1_CH1`-`ADC1_CH4`
- Zigbee transport is modeled in code now; actual ESP Zigbee cluster/endpoint binding is the next integration step
- Calibration defaults are placeholders and should be tuned using real sensor data

## Next Recommended Work

- Replace the Zigbee snapshot hook with real ESP Zigbee endpoint/cluster registration
- Add persisted calibration and threshold settings in NVS
- Add per-sensor disconnect/noise heuristics based on actual hardware behavior
- Add a manual Zigbee command path for pump start/stop that still respects safety interlocks
