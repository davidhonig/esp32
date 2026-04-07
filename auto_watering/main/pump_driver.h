#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialise the LEDC timer and channel for pump PWM output.
 */
esp_err_t pump_driver_init(void);

/**
 * @brief Switch the pump on or off.
 *
 * When turned off the PWM duty is forced to 0 regardless of the current
 * level setting; the level is remembered so it is restored on the next
 * power-on.
 *
 * @param on  true = on, false = off.
 */
void pump_driver_set_power(bool on);

/**
 * @brief Set the pump speed level.
 *
 * @param level  ZCL CurrentLevel value 0-254; maps linearly to PWM duty 0-255.
 */
void pump_driver_set_level(uint8_t level);

#ifdef __cplusplus
}
#endif
