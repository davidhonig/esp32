#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "board_config.h"

typedef enum {
    SOIL_SENSOR_HEALTH_OK = 0,
    SOIL_SENSOR_HEALTH_NOT_INITIALIZED,
    SOIL_SENSOR_HEALTH_READ_FAILED,
    SOIL_SENSOR_HEALTH_OUT_OF_RANGE,
} soil_sensor_health_t;

typedef struct {
    bool valid;
    int raw;
    int filtered_raw;
    int moisture_pct;
    soil_sensor_health_t health;
    uint32_t sample_count;
} soil_sensor_reading_t;

esp_err_t soil_sensors_init(const board_config_t *config);
esp_err_t soil_sensors_sample_all(soil_sensor_reading_t readings[SOIL_SENSOR_COUNT]);
