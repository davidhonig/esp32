#include "soil_sensors.h"

#include <limits.h>
#include <string.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "soil_sensors";

static adc_oneshot_unit_handle_t s_adc_handle;
static const board_config_t *s_config;
static soil_sensor_reading_t s_last_readings[SOIL_SENSOR_COUNT];
static bool s_initialized;

static int clamp_percent(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return value;
}

static int calculate_moisture_pct(int filtered_raw)
{
    const int numerator = (s_config->dry_reference_raw - filtered_raw) * 100;
    const int denominator = s_config->dry_reference_raw - s_config->wet_reference_raw;

    if (denominator == 0) {
        return 0;
    }

    return clamp_percent(numerator / denominator);
}

static bool is_out_of_range(int raw)
{
    const int lower_bound = s_config->wet_reference_raw - 600;
    const int upper_bound = s_config->dry_reference_raw + 600;
    return raw < lower_bound || raw > upper_bound;
}

esp_err_t soil_sensors_init(const board_config_t *config)
{
    esp_err_t err;

    s_config = config;
    memset(s_last_readings, 0, sizeof(s_last_readings));

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = config->adc_unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    err = adc_oneshot_new_unit(&init_config, &s_adc_handle);
    if (err != ESP_OK) {
        return err;
    }

    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        adc_oneshot_chan_cfg_t channel_config = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        err = adc_oneshot_config_channel(s_adc_handle, config->soil_adc_channels[i], &channel_config);
        if (err != ESP_OK) {
            return err;
        }
        s_last_readings[i].health = SOIL_SENSOR_HEALTH_NOT_INITIALIZED;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Configured %u soil sensors on ADC unit %d", SOIL_SENSOR_COUNT, config->adc_unit);
    return ESP_OK;
}

esp_err_t soil_sensors_sample_all(soil_sensor_reading_t readings[SOIL_SENSOR_COUNT])
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    for (size_t i = 0; i < SOIL_SENSOR_COUNT; ++i) {
        int raw = 0;
        esp_err_t err = adc_oneshot_read(s_adc_handle, s_config->soil_adc_channels[i], &raw);
        soil_sensor_reading_t next = s_last_readings[i];

        next.sample_count += 1;
        next.raw = raw;
        next.valid = false;

        if (err != ESP_OK) {
            next.health = SOIL_SENSOR_HEALTH_READ_FAILED;
            ESP_LOGW(TAG, "Sensor %u read failed: %s", (unsigned)i, esp_err_to_name(err));
        } else {
            if (next.sample_count == 1 || next.filtered_raw == 0) {
                next.filtered_raw = raw;
            } else {
                next.filtered_raw =
                    ((next.filtered_raw * (100 - s_config->filter_alpha_pct)) +
                     (raw * s_config->filter_alpha_pct)) /
                    100;
            }

            next.moisture_pct = calculate_moisture_pct(next.filtered_raw);
            next.health = is_out_of_range(raw) ? SOIL_SENSOR_HEALTH_OUT_OF_RANGE : SOIL_SENSOR_HEALTH_OK;
            next.valid = next.health == SOIL_SENSOR_HEALTH_OK;
        }

        s_last_readings[i] = next;
        readings[i] = next;
    }

    return ESP_OK;
}
