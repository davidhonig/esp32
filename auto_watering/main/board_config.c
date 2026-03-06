#include "board_config.h"

const board_config_t *board_config_get(void)
{
    static const board_config_t config = {
        .pump_gpio = CONFIG_AUTO_WATERING_PUMP_GPIO,
        .pump_active_high = true,
        .adc_unit = ADC_UNIT_1,
        .soil_adc_channels = {
            ADC_CHANNEL_1,
            ADC_CHANNEL_2,
            ADC_CHANNEL_3,
            ADC_CHANNEL_4,
        },
        .soil_sensor_gpios = {
            GPIO_NUM_1,
            GPIO_NUM_2,
            GPIO_NUM_3,
            GPIO_NUM_4,
        },
        .sample_period_ms = CONFIG_AUTO_WATERING_SAMPLE_PERIOD_MS,
        .pump_max_runtime_ms = CONFIG_AUTO_WATERING_PUMP_MAX_RUNTIME_MS,
        .pump_lockout_ms = CONFIG_AUTO_WATERING_PUMP_LOCKOUT_MS,
        .auto_water_runtime_ms = CONFIG_AUTO_WATERING_AUTO_WATER_RUNTIME_MS,
        .filter_alpha_pct = CONFIG_AUTO_WATERING_FILTER_ALPHA_PCT,
        .dry_threshold_pct = CONFIG_AUTO_WATERING_DRY_THRESHOLD_PCT,
        .wet_reference_raw = CONFIG_AUTO_WATERING_WET_REFERENCE_RAW,
        .dry_reference_raw = CONFIG_AUTO_WATERING_DRY_REFERENCE_RAW,
        .automation_enabled = CONFIG_AUTO_WATERING_ENABLE_AUTOMATION,
    };

    return &config;
}
