#include "pump_control.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "pump_control";

static const board_config_t *s_config;
static pump_status_t s_status;

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static void set_output_level(bool on)
{
    const int level = (on == s_config->pump_active_high) ? 1 : 0;
    gpio_set_level(s_config->pump_gpio, level);
}

esp_err_t pump_control_init(const board_config_t *config)
{
    s_config = config;
    s_status = (pump_status_t){0};

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << config->pump_gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return err;
    }

    set_output_level(false);
    s_status.initialized = true;
    ESP_LOGI(TAG, "Pump GPIO %d configured and forced OFF", config->pump_gpio);
    return ESP_OK;
}

esp_err_t pump_control_start(uint32_t runtime_ms)
{
    const uint64_t current_ms = now_ms();

    if (!s_status.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (runtime_ms == 0 || runtime_ms > s_config->pump_max_runtime_ms) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_status.running) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((current_ms - s_status.last_stop_ms) < s_config->pump_lockout_ms && s_status.last_stop_ms != 0) {
        s_status.lockout_active = true;
        return ESP_ERR_INVALID_STATE;
    }

    set_output_level(true);
    s_status.running = true;
    s_status.lockout_active = false;
    s_status.runtime_ms = runtime_ms;
    s_status.last_start_ms = current_ms;
    s_status.stop_deadline_ms = current_ms + runtime_ms;
    ESP_LOGW(TAG, "Pump started for %u ms", runtime_ms);
    return ESP_OK;
}

void pump_control_stop(void)
{
    if (!s_status.initialized) {
        return;
    }
    if (!s_status.running) {
        return;
    }

    set_output_level(false);
    s_status.running = false;
    s_status.last_stop_ms = now_ms();
    s_status.stop_deadline_ms = 0;
    s_status.runtime_ms = 0;
    s_status.lockout_active = true;
    ESP_LOGW(TAG, "Pump stopped; lockout active for %u ms", s_config->pump_lockout_ms);
}

void pump_control_process(void)
{
    const uint64_t current_ms = now_ms();

    if (!s_status.initialized) {
        return;
    }

    if (s_status.running && current_ms >= s_status.stop_deadline_ms) {
        pump_control_stop();
        return;
    }

    if (!s_status.running && s_status.last_stop_ms != 0 &&
        (current_ms - s_status.last_stop_ms) >= s_config->pump_lockout_ms) {
        s_status.lockout_active = false;
    }
}

bool pump_control_is_running(void)
{
    return s_status.running;
}

bool pump_control_is_lockout_active(void)
{
    return s_status.lockout_active;
}

uint32_t pump_control_remaining_runtime_ms(void)
{
    const uint64_t current_ms = now_ms();

    if (!s_status.running || current_ms >= s_status.stop_deadline_ms) {
        return 0;
    }

    return (uint32_t)(s_status.stop_deadline_ms - current_ms);
}

const pump_status_t *pump_control_get_status(void)
{
    return &s_status;
}
