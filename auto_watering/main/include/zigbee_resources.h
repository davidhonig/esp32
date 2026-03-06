#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "soil_sensors.h"
#include "watering_logic.h"

#define ZB_ENDPOINT_CONTROLLER 0x01
#define ZB_ENDPOINT_SENSOR_BASE 0x10
#define ZB_ENDPOINT_PUMP 0x20
#define ZB_ENDPOINT_DIAGNOSTICS 0x21

typedef struct {
    uint8_t endpoint_id;
    char name[24];
    int moisture_pct;
    int raw;
    bool valid;
    soil_sensor_health_t health;
} zigbee_soil_resource_t;

typedef struct {
    uint8_t endpoint_id;
    bool running;
    bool lockout_active;
    uint32_t remaining_runtime_ms;
} zigbee_pump_resource_t;

typedef struct {
    uint8_t endpoint_id;
    bool automation_enabled;
    bool sensor_fault_active;
    uint8_t driest_sensor_index;
    int driest_moisture_pct;
    uint64_t loop_count;
} zigbee_controller_resource_t;

typedef struct {
    uint8_t endpoint_id;
    uint32_t uptime_sec;
    bool zigbee_enabled;
} zigbee_diagnostics_resource_t;

typedef struct {
    zigbee_soil_resource_t sensors[SOIL_SENSOR_COUNT];
    zigbee_pump_resource_t pump;
    zigbee_controller_resource_t controller;
    zigbee_diagnostics_resource_t diagnostics;
} zigbee_resource_snapshot_t;

void zigbee_resources_init(void);
void zigbee_resources_sync(const soil_sensor_reading_t readings[SOIL_SENSOR_COUNT],
                           const watering_status_t *status,
                           uint32_t uptime_sec);
esp_err_t zigbee_resources_handle_manual_pump_request(uint32_t runtime_ms);
void zigbee_resources_set_automation_enabled(bool enabled);
const zigbee_resource_snapshot_t *zigbee_resources_get_snapshot(void);
