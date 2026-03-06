#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "board_config.h"

typedef struct {
    bool initialized;
    bool running;
    bool lockout_active;
    uint32_t runtime_ms;
    uint64_t last_start_ms;
    uint64_t stop_deadline_ms;
    uint64_t last_stop_ms;
} pump_status_t;

esp_err_t pump_control_init(const board_config_t *config);
esp_err_t pump_control_start(uint32_t runtime_ms);
void pump_control_stop(void);
void pump_control_process(void);
bool pump_control_is_running(void);
bool pump_control_is_lockout_active(void);
uint32_t pump_control_remaining_runtime_ms(void);
const pump_status_t *pump_control_get_status(void);
