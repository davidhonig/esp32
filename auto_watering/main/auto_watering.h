#pragma once

#include "esp_zigbee_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Zigbee network config ─────────────────────────────────────────────── */
#define INSTALLCODE_POLICY_ENABLE   false
#define ED_AGING_TIMEOUT            ESP_ZB_ED_AGING_TIMEOUT_64MIN
#define ED_KEEP_ALIVE               3000   /* ms */
#define ESP_ZB_PRIMARY_CHANNEL_MASK ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK

/* ── Manufacturer / model strings ──────────────────────────────────────── */
#define ESP_MANUFACTURER_NAME  "\x09""ESPRESSIF"
#define ESP_MODEL_IDENTIFIER   "\x0d""auto_watering"

/* ── Endpoint layout ───────────────────────────────────────────────────── */
#define NUM_SENSORS           4
#define SENSOR_ENDPOINT_BASE  1    /* endpoints 1-4 → sensors (humidity cluster 0x0405) */
#define PUMP_ENDPOINT         5    /* endpoint  5   → pump    */

/* ── Pump timed-run controls (standard ON/OFF OnTime, 0.1 s units) ─────── */
#define PUMP_RUN_SECONDS_DEFAULT  10
#define PUMP_RUN_SECONDS_MAX      3600
#define PUMP_RUN_ON_TIME_DEFAULT_DS  ((uint16_t)(PUMP_RUN_SECONDS_DEFAULT * 10U))
#define PUMP_RUN_ON_TIME_MAX_DS      ((uint16_t)(PUMP_RUN_SECONDS_MAX * 10U))
#define PUMP_RUN_DURATION_SETTING_DEFAULT_DS  ((uint8_t)10U)
#define PUMP_RUN_DURATION_SETTING_MAX_DS      ((uint8_t)254U)

/* ── GPIO assignments ──────────────────────────────────────────────────── */
#define BOOT_BUTTON_GPIO  9  /* Waveshare ESP32-H2-Zero BOOT button (active-low) */
#define SENSOR_1_GPIO  0   /* ADC1_CH0 */
#define SENSOR_2_GPIO  1   /* ADC1_CH1 */
#define SENSOR_3_GPIO  2   /* ADC1_CH2 */
#define SENSOR_4_GPIO  3   /* ADC1_CH3 */
#define PUMP_GPIO      10  /* LEDC PWM output */

/* ── ADC channels (ESP32-H2 ADC1, GPIOx → ADC_CHANNEL_x) ─────────────── */
#define SENSOR_ADC_CHANNEL_1  ADC_CHANNEL_0
#define SENSOR_ADC_CHANNEL_2  ADC_CHANNEL_1
#define SENSOR_ADC_CHANNEL_3  ADC_CHANNEL_2
#define SENSOR_ADC_CHANNEL_4  ADC_CHANNEL_3

/* ── LEDC (PWM) pump config ────────────────────────────────────────────── */
#define PUMP_LEDC_MODE      LEDC_LOW_SPEED_MODE  /* ESP32-H2 only has low-speed */
#define PUMP_LEDC_TIMER     LEDC_TIMER_0
#define PUMP_LEDC_CHANNEL   LEDC_CHANNEL_0
#define PUMP_LEDC_DUTY_RES  LEDC_TIMER_8_BIT     /* 0-255 maps to ZCL 0-254 */
#define PUMP_LEDC_FREQ_HZ   1000
#define PUMP_LEVEL_PWM_MIN    80
#define PUMP_LEVEL_PWM_MAX    254
#define PUMP_LEVEL_ZCL_MAX    254 /* standard Zigbee Level Control range */

/* ── Sensor polling period ─────────────────────────────────────────────── */
#define SENSOR_POLL_INTERVAL_MS  10000

/* ── Moisture ADC calibration / filtering ──────────────────────────────── */
#define SENSOR_DRY_MV            2800  /* dry soil sensor output */
#define SENSOR_WET_MV            1060  /* fully wet soil sensor output */
#define SENSOR_ADC_SAMPLES       15    /* median sample count per read */
#define SENSOR_FILTER_ALPHA_PCT  25    /* EMA alpha in percent (1-100) */

/* ── Zigbee stack helper macros (mirrors ha_on_off_light pattern) ──────── */
#define ESP_ZB_ZED_CONFIG()                                      \
    {                                                            \
        .esp_zb_role          = ESP_ZB_DEVICE_TYPE_ED,           \
        .install_code_policy  = INSTALLCODE_POLICY_ENABLE,       \
        .nwk_cfg.zed_cfg = {                                     \
            .ed_timeout   = ED_AGING_TIMEOUT,                    \
            .keep_alive   = ED_KEEP_ALIVE,                       \
        },                                                       \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                            \
    {                                                            \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                      \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                             \
    {                                                            \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,    \
    }

/* Fallback device-ID define in case the SDK version doesn't expose it */
#ifndef ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID
#define ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID  0x000C
#endif

#ifdef __cplusplus
}
#endif
