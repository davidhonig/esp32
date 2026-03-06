#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define SOIL_SENSOR_COUNT 4

#ifndef CONFIG_AUTO_WATERING_PUMP_GPIO
#define CONFIG_AUTO_WATERING_PUMP_GPIO 10
#endif

#ifndef CONFIG_AUTO_WATERING_SAMPLE_PERIOD_MS
#define CONFIG_AUTO_WATERING_SAMPLE_PERIOD_MS 5000
#endif

#ifndef CONFIG_AUTO_WATERING_FILTER_ALPHA_PCT
#define CONFIG_AUTO_WATERING_FILTER_ALPHA_PCT 25
#endif

#ifndef CONFIG_AUTO_WATERING_DRY_THRESHOLD_PCT
#define CONFIG_AUTO_WATERING_DRY_THRESHOLD_PCT 35
#endif

#ifndef CONFIG_AUTO_WATERING_WET_REFERENCE_RAW
#define CONFIG_AUTO_WATERING_WET_REFERENCE_RAW 1200
#endif

#ifndef CONFIG_AUTO_WATERING_DRY_REFERENCE_RAW
#define CONFIG_AUTO_WATERING_DRY_REFERENCE_RAW 2800
#endif

#ifndef CONFIG_AUTO_WATERING_PUMP_MAX_RUNTIME_MS
#define CONFIG_AUTO_WATERING_PUMP_MAX_RUNTIME_MS 5000
#endif

#ifndef CONFIG_AUTO_WATERING_PUMP_LOCKOUT_MS
#define CONFIG_AUTO_WATERING_PUMP_LOCKOUT_MS 60000
#endif

#ifndef CONFIG_AUTO_WATERING_AUTO_WATER_RUNTIME_MS
#define CONFIG_AUTO_WATERING_AUTO_WATER_RUNTIME_MS 3000
#endif

#ifndef CONFIG_AUTO_WATERING_ENABLE_AUTOMATION
#define CONFIG_AUTO_WATERING_ENABLE_AUTOMATION 1
#endif

typedef struct {
    gpio_num_t pump_gpio;
    bool pump_active_high;
    adc_unit_t adc_unit;
    adc_channel_t soil_adc_channels[SOIL_SENSOR_COUNT];
    gpio_num_t soil_sensor_gpios[SOIL_SENSOR_COUNT];
    uint32_t sample_period_ms;
    uint32_t pump_max_runtime_ms;
    uint32_t pump_lockout_ms;
    uint32_t auto_water_runtime_ms;
    uint8_t filter_alpha_pct;
    uint8_t dry_threshold_pct;
    int wet_reference_raw;
    int dry_reference_raw;
    bool automation_enabled;
} board_config_t;

const board_config_t *board_config_get(void);
