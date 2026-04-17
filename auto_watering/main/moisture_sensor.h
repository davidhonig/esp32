#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise ADC unit and configure all four sensor channels.
 */
esp_err_t moisture_sensor_init(void);

/**
 * @brief Read one soil-moisture sensor.
 *
 * Capacitive sensors output high voltage when dry and lower voltage when wet.
 * The driver takes multiple ADC samples, uses the median to reject spikes,
 * converts to mV (calibrated when available), maps dry/wet voltage points to
 * 0-10000, and applies EMA smoothing for stable Zigbee reporting.
 *
 * @param sensor_idx  Sensor index 0-3.
 * @return            Moisture value 0-10000, or 0 on read error.
 */
uint16_t moisture_sensor_read(int sensor_idx);

#ifdef __cplusplus
}
#endif
