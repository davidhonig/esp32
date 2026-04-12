#include "pump_driver.h"
#include "auto_watering.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "PUMP_DRIVER";

static bool    s_pump_on    = false;
static uint8_t s_pump_level = 0;

esp_err_t pump_driver_init(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = PUMP_LEDC_MODE,
        .duty_resolution = PUMP_LEDC_DUTY_RES,
        .timer_num       = PUMP_LEDC_TIMER,
        .freq_hz         = PUMP_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "LEDC timer config failed");

    ledc_channel_config_t chan_cfg = {
        .gpio_num   = PUMP_GPIO,
        .speed_mode = PUMP_LEDC_MODE,
        .channel    = PUMP_LEDC_CHANNEL,
        .timer_sel  = PUMP_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&chan_cfg), TAG, "LEDC channel config failed");

    ESP_LOGI(TAG, "Pump LEDC initialised (GPIO %d, %u Hz, 8-bit)", PUMP_GPIO, PUMP_LEDC_FREQ_HZ);
    return ESP_OK;
}

static void apply_pwm(void)
{
    uint32_t duty = s_pump_on ? (uint32_t)s_pump_level : 0u;
    ledc_set_duty(PUMP_LEDC_MODE, PUMP_LEDC_CHANNEL, duty);
    ledc_update_duty(PUMP_LEDC_MODE, PUMP_LEDC_CHANNEL);
    ESP_LOGI(TAG, "Pump PWM duty=%lu  (%s, level=%u)", duty,
             s_pump_on ? "ON" : "OFF", s_pump_level);
}

void pump_driver_set_power(bool on)
{
    s_pump_on = on;
    apply_pwm();
}

void pump_driver_set_level(uint8_t level)
{
    s_pump_level = level;
    apply_pwm();
}
