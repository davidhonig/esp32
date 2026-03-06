#include "watering_logic.h"

#include <limits.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "pump_control.h"

static const char *TAG = "watering_logic";

static const board_config_t *s_config;
static soil_sensor_reading_t s_readings[SOIL_SENSOR_COUNT];
static watering_status_t s_status;
static uint64_t s_last_sample_ms;
static bool s_initialized;

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static bool readings_have_fault(const soil_sensor_reading_t readings[SOIL_SENSOR_COUNT])
{
    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        if (!readings[i].valid) {
            return true;
        }
    }
    return false;
}

static void update_driest_sensor(void)
{
    int driest = INT_MAX;
    uint8_t driest_index = 0;

    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        if (!s_readings[i].valid) {
            continue;
        }
        if (s_readings[i].moisture_pct < driest) {
            driest = s_readings[i].moisture_pct;
            driest_index = (uint8_t)i;
        }
    }

    if (driest == INT_MAX) {
        s_status.driest_sensor_index = 0;
        s_status.driest_moisture_pct = -1;
    } else {
        s_status.driest_sensor_index = driest_index;
        s_status.driest_moisture_pct = driest;
    }
}

esp_err_t watering_logic_init(const board_config_t *config)
{
    s_config = config;
    s_status = (watering_status_t){
        .automation_enabled = config->automation_enabled,
        .sample_period_ms = config->sample_period_ms,
        .driest_moisture_pct = -1,
    };
    memset(s_readings, 0, sizeof(s_readings));
    s_last_sample_ms = 0;
    s_initialized = true;
    ESP_LOGI(TAG, "Automation %s, dry threshold %u%%",
             s_status.automation_enabled ? "enabled" : "disabled",
             config->dry_threshold_pct);
    return ESP_OK;
}

esp_err_t watering_logic_request_manual_run(uint32_t runtime_ms)
{
    ESP_LOGI(TAG, "Manual watering requested for %u ms", runtime_ms);
    return pump_control_start(runtime_ms);
}

void watering_logic_set_automation(bool enabled)
{
    s_status.automation_enabled = enabled;
    ESP_LOGI(TAG, "Automation %s", enabled ? "enabled" : "disabled");
}

esp_err_t watering_logic_poll(void)
{
    const uint64_t current_ms = now_ms();

    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    pump_control_process();
    s_status.loop_count += 1;
    s_status.pump_running = pump_control_is_running();
    s_status.lockout_active = pump_control_is_lockout_active();

    if (s_last_sample_ms != 0 && (current_ms - s_last_sample_ms) < s_config->sample_period_ms) {
        return ESP_OK;
    }

    s_last_sample_ms = current_ms;

    esp_err_t err = soil_sensors_sample_all(s_readings);
    if (err != ESP_OK) {
        return err;
    }

    s_status.sensor_fault_active = readings_have_fault(s_readings);
    update_driest_sensor();

    if (s_status.sensor_fault_active) {
        if (pump_control_is_running()) {
            ESP_LOGW(TAG, "Stopping pump because a sensor fault is active");
            pump_control_stop();
            s_status.pump_running = false;
        }
        return ESP_OK;
    }

    if (!s_status.automation_enabled || s_status.pump_running || s_status.lockout_active) {
        return ESP_OK;
    }

    if (s_status.driest_moisture_pct >= 0 &&
        s_status.driest_moisture_pct <= s_config->dry_threshold_pct) {
        ESP_LOGI(TAG, "Auto-watering triggered by sensor %u at %d%% moisture",
                 s_status.driest_sensor_index,
                 s_status.driest_moisture_pct);
        err = pump_control_start(s_config->auto_water_runtime_ms);
        if (err == ESP_OK) {
            s_status.pump_running = true;
            s_status.lockout_active = pump_control_is_lockout_active();
        } else {
            ESP_LOGW(TAG, "Auto-watering request rejected: %s", esp_err_to_name(err));
        }
    }

    return ESP_OK;
}

const soil_sensor_reading_t *watering_logic_get_sensor_readings(void)
{
    return s_readings;
}

const watering_status_t *watering_logic_get_status(void)
{
    s_status.pump_running = pump_control_is_running();
    s_status.lockout_active = pump_control_is_lockout_active();
    return &s_status;
}
