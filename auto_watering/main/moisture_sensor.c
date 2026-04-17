#include "moisture_sensor.h"
#include "auto_watering.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "MOISTURE_SENSOR";

static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t s_adc_cali_handle;
static bool s_adc_cali_enabled;
static uint16_t s_filtered_moisture[NUM_SENSORS];
static bool s_filter_valid[NUM_SENSORS];

static const adc_channel_t s_channels[NUM_SENSORS] = {
    SENSOR_ADC_CHANNEL_1,
    SENSOR_ADC_CHANNEL_2,
    SENSOR_ADC_CHANNEL_3,
    SENSOR_ADC_CHANNEL_4,
};

static void sort_int_array(int *values, int count)
{
    for (int i = 1; i < count; i++) {
        int key = values[i];
        int j = i - 1;
        while (j >= 0 && values[j] > key) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = key;
    }
}

static esp_err_t read_sensor_raw_median(int sensor_idx, int *median_raw)
{
    int samples[SENSOR_ADC_SAMPLES];

    for (int i = 0; i < SENSOR_ADC_SAMPLES; i++) {
        esp_err_t ret = adc_oneshot_read(s_adc_handle, s_channels[sensor_idx], &samples[i]);
        if (ret != ESP_OK) {
            return ret;
        }
    }

    sort_int_array(samples, SENSOR_ADC_SAMPLES);
    *median_raw = samples[SENSOR_ADC_SAMPLES / 2];
    return ESP_OK;
}

static uint16_t moisture_from_mv(uint32_t voltage_mv)
{
    if (voltage_mv >= SENSOR_DRY_MV) {
        return 0;
    }
    if (voltage_mv <= SENSOR_WET_MV) {
        return 10000;
    }

    uint32_t span_mv = (uint32_t)(SENSOR_DRY_MV - SENSOR_WET_MV);
    uint32_t moisture = ((uint32_t)(SENSOR_DRY_MV - voltage_mv) * 10000U + (span_mv / 2U)) / span_mv;
    if (moisture > 10000U) {
        moisture = 10000U;
    }
    return (uint16_t)moisture;
}

static uint16_t filter_moisture(int sensor_idx, uint16_t instant_moisture)
{
    if (!s_filter_valid[sensor_idx]) {
        s_filtered_moisture[sensor_idx] = instant_moisture;
        s_filter_valid[sensor_idx] = true;
        return instant_moisture;
    }

    uint32_t alpha = SENSOR_FILTER_ALPHA_PCT;
    if (alpha == 0) {
        return s_filtered_moisture[sensor_idx];
    }
    if (alpha > 100) {
        alpha = 100;
    }

    uint32_t prev = s_filtered_moisture[sensor_idx];
    uint32_t filtered = (prev * (100U - alpha) + (uint32_t)instant_moisture * alpha + 50U) / 100U;
    s_filtered_moisture[sensor_idx] = (uint16_t)filtered;
    return s_filtered_moisture[sensor_idx];
}

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
        s_filter_valid[i] = false;
        s_filtered_moisture[i] = 0;
    }

    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .chan = s_channels[0],
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    esp_err_t cali_ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_adc_cali_handle);
    if (cali_ret == ESP_OK) {
        s_adc_cali_enabled = true;
        ESP_LOGI(TAG, "ADC calibration enabled (curve fitting)");
    } else {
        s_adc_cali_enabled = false;
        ESP_LOGW(TAG, "ADC calibration unavailable (%s), using raw->mV fallback",
                 esp_err_to_name(cali_ret));
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
    esp_err_t ret = read_sensor_raw_median(sensor_idx, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Sensor %d ADC read error: %s", sensor_idx, esp_err_to_name(ret));
        return 0;
    }

    uint32_t voltage_mv = ((uint32_t)raw * 3300U + 2047U) / 4095U;
    if (s_adc_cali_enabled) {
        int calibrated_mv = 0;
        if (adc_cali_raw_to_voltage(s_adc_cali_handle, raw, &calibrated_mv) == ESP_OK &&
            calibrated_mv >= 0) {
            voltage_mv = (uint32_t)calibrated_mv;
        }
    }

    uint16_t instant_moisture = moisture_from_mv(voltage_mv);
    uint16_t filtered_moisture = filter_moisture(sensor_idx, instant_moisture);

    ESP_LOGD(TAG, "Sensor %d: raw=%d mv=%u instant=%u filtered=%u",
             sensor_idx, raw, (unsigned)voltage_mv,
             (unsigned)instant_moisture, (unsigned)filtered_moisture);
    return filtered_moisture;
}
