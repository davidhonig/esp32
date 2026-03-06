#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "board_config.h"
#include "soil_sensors.h"

typedef struct {
    bool automation_enabled;
    bool sensor_fault_active;
    bool pump_running;
    bool lockout_active;
    uint8_t driest_sensor_index;
    int driest_moisture_pct;
    uint32_t sample_period_ms;
    uint64_t loop_count;
} watering_status_t;

esp_err_t watering_logic_init(const board_config_t *config);
esp_err_t watering_logic_poll(void);
esp_err_t watering_logic_request_manual_run(uint32_t runtime_ms);
void watering_logic_set_automation(bool enabled);
const soil_sensor_reading_t *watering_logic_get_sensor_readings(void);
const watering_status_t *watering_logic_get_status(void);
