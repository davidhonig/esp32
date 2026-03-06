#include <inttypes.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "board_config.h"
#include "pump_control.h"
#include "soil_sensors.h"
#include "watering_logic.h"
#include "zigbee_resources.h"

static const char *TAG = "app_main";

static uint32_t uptime_sec(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

void app_main(void)
{
    const board_config_t *config = board_config_get();

    init_nvs();
    ESP_ERROR_CHECK(pump_control_init(config));
    ESP_ERROR_CHECK(soil_sensors_init(config));
    ESP_ERROR_CHECK(watering_logic_init(config));
    zigbee_resources_init();

    ESP_LOGI(TAG, "Auto-watering started: sensors=GPIO1-4, pump GPIO=%d, auto=%s",
             config->pump_gpio,
             config->automation_enabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "GPIO0 reserved for fast SPI, GPIO10-14 available for PWM outputs");

    while (true) {
        esp_err_t err = watering_logic_poll();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "watering_logic_poll failed: %s", esp_err_to_name(err));
        }

        const watering_status_t *status = watering_logic_get_status();
        const soil_sensor_reading_t *readings = watering_logic_get_sensor_readings();
        zigbee_resources_sync(readings, status, uptime_sec());

        ESP_LOGI(TAG,
                 "driest=s%u:%d%% pump=%s lockout=%s fault=%s remaining=%u ms loops=%" PRIu64,
                 status->driest_sensor_index,
                 status->driest_moisture_pct,
                 status->pump_running ? "on" : "off",
                 status->lockout_active ? "yes" : "no",
                 status->sensor_fault_active ? "yes" : "no",
                 pump_control_remaining_runtime_ms(),
                 status->loop_count);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
