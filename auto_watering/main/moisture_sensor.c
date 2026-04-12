#include "moisture_sensor.h"
#include "auto_watering.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "MOISTURE_SENSOR";

static adc_oneshot_unit_handle_t s_adc_handle;

static const adc_channel_t s_channels[NUM_SENSORS] = {
    SENSOR_ADC_CHANNEL_1,
    SENSOR_ADC_CHANNEL_2,
    SENSOR_ADC_CHANNEL_3,
    SENSOR_ADC_CHANNEL_4,
};

esp_err_t moisture_sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle),
                        TAG, "ADC unit init failed");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,   /* full 0-3.3 V range */
    };
    for (int i = 0; i < NUM_SENSORS; i++) {
        ESP_RETURN_ON_ERROR(
            adc_oneshot_config_channel(s_adc_handle, s_channels[i], &chan_cfg),
            TAG, "ADC channel %d config failed", i);
    }

    ESP_LOGI(TAG, "Moisture sensor ADC initialised (%d channels)", NUM_SENSORS);
    return ESP_OK;
}

uint16_t moisture_sensor_read(int sensor_idx)
{
    if (sensor_idx < 0 || sensor_idx >= NUM_SENSORS) {
        ESP_LOGW(TAG, "Invalid sensor index %d", sensor_idx);
        return 0;
    }

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, s_channels[sensor_idx], &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Sensor %d ADC read error: %s", sensor_idx, esp_err_to_name(ret));
        return 0;
    }

    /* Invert and scale: dry → high ADC raw (~4095) → low moisture (0)
     *                   wet → low  ADC raw (~0)    → high moisture (10000) */
    uint16_t moisture = (uint16_t)((uint32_t)(4095 - raw) * 10000u / 4095u);
    ESP_LOGD(TAG, "Sensor %d: raw=%d  moisture=%u/10000", sensor_idx, raw, moisture);
    return moisture;
}
