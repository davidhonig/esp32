#include "zigbee_resources.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "pump_control.h"

static const char *TAG = "zigbee_resources";

static zigbee_resource_snapshot_t s_snapshot;

void zigbee_resources_init(void)
{
    memset(&s_snapshot, 0, sizeof(s_snapshot));

    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        s_snapshot.sensors[i].endpoint_id = ZB_ENDPOINT_SENSOR_BASE + (uint8_t)i;
        snprintf(s_snapshot.sensors[i].name, sizeof(s_snapshot.sensors[i].name), "soil_sensor_%u", (unsigned)i);
    }

    s_snapshot.pump.endpoint_id = ZB_ENDPOINT_PUMP;
    s_snapshot.controller.endpoint_id = ZB_ENDPOINT_CONTROLLER;
    s_snapshot.diagnostics.endpoint_id = ZB_ENDPOINT_DIAGNOSTICS;

    ESP_LOGI(TAG, "Resource model initialized: controller=0x%02x pump=0x%02x diagnostics=0x%02x",
             ZB_ENDPOINT_CONTROLLER,
             ZB_ENDPOINT_PUMP,
             ZB_ENDPOINT_DIAGNOSTICS);
}

void zigbee_resources_sync(const soil_sensor_reading_t readings[SOIL_SENSOR_COUNT],
                           const watering_status_t *status,
                           uint32_t uptime_sec)
{
    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        s_snapshot.sensors[i].moisture_pct = readings[i].moisture_pct;
        s_snapshot.sensors[i].raw = readings[i].raw;
        s_snapshot.sensors[i].valid = readings[i].valid;
        s_snapshot.sensors[i].health = readings[i].health;
    }

    s_snapshot.pump.running = pump_control_is_running();
    s_snapshot.pump.lockout_active = pump_control_is_lockout_active();
    s_snapshot.pump.remaining_runtime_ms = pump_control_remaining_runtime_ms();

    s_snapshot.controller.automation_enabled = status->automation_enabled;
    s_snapshot.controller.sensor_fault_active = status->sensor_fault_active;
    s_snapshot.controller.driest_sensor_index = status->driest_sensor_index;
    s_snapshot.controller.driest_moisture_pct = status->driest_moisture_pct;
    s_snapshot.controller.loop_count = status->loop_count;

    s_snapshot.diagnostics.uptime_sec = uptime_sec;
#if CONFIG_AUTO_WATERING_ENABLE_ZIGBEE
    s_snapshot.diagnostics.zigbee_enabled = true;
    ESP_LOGD(TAG, "Zigbee hooks enabled; publish controller=%d%% sensor=%u pump=%s",
             status->driest_moisture_pct,
             status->driest_sensor_index,
             s_snapshot.pump.running ? "on" : "off");
#else
    s_snapshot.diagnostics.zigbee_enabled = false;
#endif
}

esp_err_t zigbee_resources_handle_manual_pump_request(uint32_t runtime_ms)
{
    ESP_LOGI(TAG, "Zigbee manual pump request: %u ms", runtime_ms);
    return watering_logic_request_manual_run(runtime_ms);
}

void zigbee_resources_set_automation_enabled(bool enabled)
{
    ESP_LOGI(TAG, "Zigbee automation set: %s", enabled ? "enabled" : "disabled");
    watering_logic_set_automation(enabled);
}

const zigbee_resource_snapshot_t *zigbee_resources_get_snapshot(void)
{
    return &s_snapshot;
}
