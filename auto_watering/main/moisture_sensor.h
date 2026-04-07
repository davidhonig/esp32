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
 * Capacitive sensors output high voltage (~3.3 V) when dry and low voltage
 * when wet.  The raw ADC value is inverted and scaled to the Zigbee
 * Soil Moisture Measurement cluster range: 0 = 0 %, 10000 = 100 %.
 *
 * @param sensor_idx  Sensor index 0-3.
 * @return            Moisture value 0-10000, or 0 on read error.
 */
uint16_t moisture_sensor_read(int sensor_idx);

#ifdef __cplusplus
}
#endif
