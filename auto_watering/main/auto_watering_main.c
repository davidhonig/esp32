/*
 * auto_watering_main.c
 *
 * Zigbee End Device exposing:
 *   Endpoints 1-4 : Relative Humidity Measurement (0x0405) used for soil moisture
 *   Endpoint  5   : Water Pump – ON/OFF (0x0006) + Level Control (0x0008)
 *
 * Board: Waveshare ESP32-H2-Zero
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_zigbee_attribute.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "esp_zigbee_cluster.h"
#include "zcl/esp_zigbee_zcl_on_off.h"
#include "zcl/esp_zigbee_zcl_command.h"
#include "zcl/esp_zigbee_zcl_level.h"

#include "auto_watering.h"
#include "moisture_sensor.h"
#include "pump_driver.h"

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE in idf.py menuconfig to compile auto_watering (End Device).
#endif

static const char *TAG = "AUTO_WATERING";
static uint8_t s_pump_level_attr = 0;
static bool s_pump_on_off_attr = false;

static TaskHandle_t s_button_task_handle;

/* ── Forward declarations ──────────────────────────────────────────────── */
static void add_moisture_endpoint(esp_zb_ep_list_t *ep_list, uint8_t ep_id);
static void add_pump_endpoint(esp_zb_ep_list_t *ep_list, uint8_t ep_id);

/* ── Boot button ───────────────────────────────────────────────────────── */

static void IRAM_ATTR boot_button_isr(void *arg)
{
    BaseType_t higher_prio_woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_button_task_handle, &higher_prio_woken);
    portYIELD_FROM_ISR(higher_prio_woken);
}

static void button_task(void *pvParameters)
{
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   /* wait for ISR */
        vTaskDelay(pdMS_TO_TICKS(50));             /* debounce */
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {
            ESP_LOGI(TAG, "BOOT button pressed – factory reset and rejoin");
            esp_zb_factory_reset();
        }
    }
}

static esp_err_t boot_button_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask  = 1ULL << BOOT_BUTTON_GPIO,
        .mode          = GPIO_MODE_INPUT,
        .pull_up_en    = GPIO_PULLUP_ENABLE,
        .pull_down_en  = GPIO_PULLDOWN_DISABLE,
        .intr_type     = GPIO_INTR_NEGEDGE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG, "Boot button GPIO config failed");
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "GPIO ISR service install failed");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(BOOT_BUTTON_GPIO, boot_button_isr, NULL),
                        TAG, "Boot button ISR handler add failed");
    return ESP_OK;
}

static uint8_t pump_level_to_pwm(uint8_t level)
{
    if (level == 0) {
        return 0;
    }

    uint16_t pwm_span = (uint16_t)(PUMP_LEVEL_PWM_MAX - PUMP_LEVEL_PWM_MIN);
    return (uint8_t)(PUMP_LEVEL_PWM_MIN +
                     (uint16_t)(((uint16_t)level * pwm_span + (PUMP_LEVEL_ZCL_MAX / 2U)) /
                                PUMP_LEVEL_ZCL_MAX));
}

static esp_zb_attribute_list_t *create_basic_cluster(void)
{
    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = 0x01,
    };
    esp_zb_attribute_list_t *basic_cluster = esp_zb_basic_cluster_create(&basic_cfg);
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
        basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, (void *)ESP_MANUFACTURER_NAME));
    ESP_ERROR_CHECK(esp_zb_basic_cluster_add_attr(
        basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, (void *)ESP_MODEL_IDENTIFIER));
    return basic_cluster;
}

static void report_moisture_value(uint8_t endpoint)
{
    esp_zb_zcl_report_attr_cmd_t report = {
        .zcl_basic_cmd = {
            .dst_addr_u.addr_short = 0x0000,
            .dst_endpoint = 0,
            .src_endpoint = endpoint,
        },
        .address_mode = ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT,
        .clusterID = ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
        .attributeID = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
        .direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI,
        .manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC,
    };

    esp_err_t err = esp_zb_zcl_report_attr_cmd_req(&report);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Humidity report request failed for ep %u: %s", endpoint, esp_err_to_name(err));
    }
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  Zigbee signal handler                                                  */
/* ─────────────────────────────────────────────────────────────────────── */

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, ,
                        TAG, "Failed to start Zigbee commissioning");
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t                *p_sg_p      = signal_struct->p_app_signal;
    esp_err_t                err_status  = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type    = *p_sg_p;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;

    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode",
                     esp_zb_bdb_is_factory_new() ? "" : "non-");
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
            }
        } else {
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)",
                     esp_err_to_name(err_status));
        }
        break;

    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG,
                     "Joined network (EXT PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x "
                     "PAN: 0x%04hx  Ch: %d  Addr: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5],
                     extended_pan_id[4], extended_pan_id[3], extended_pan_id[2],
                     extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(),
                     esp_zb_get_short_address());
        } else {
            ESP_LOGI(TAG, "Network steering not successful (status: %s)",
                     esp_err_to_name(err_status));
            esp_zb_scheduler_alarm(
                (esp_zb_callback_t)bdb_start_top_level_commissioning_cb,
                ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;

    case ESP_ZB_ZDO_SIGNAL_LEAVE:
    case ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION:
        ESP_LOGI(TAG, "Left Zigbee network");
        break;

    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s",
                 esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  Attribute write handler (coordinator → device)                        */
/* ─────────────────────────────────────────────────────────────────────── */

static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS,
                        ESP_ERR_INVALID_ARG, TAG,
                        "Received message: error status(%d)", message->info.status);

    ESP_LOGI(TAG, "Attr write: endpoint(%d) cluster(0x%04x) attr(0x%04x) size(%d)",
             message->info.dst_endpoint, message->info.cluster,
             message->attribute.id, message->attribute.data.size);

    if (message->info.dst_endpoint != PUMP_ENDPOINT) {
        return ESP_OK;   /* sensor endpoints are read-only */
    }

    /* ── Endpoint 5: Pump ON/OFF ─────────────────────────────────────── */
    if (message->info.dst_endpoint == PUMP_ENDPOINT &&
        message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
        if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID &&
            message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
            bool on = message->attribute.data.value
                      ? *(bool *)message->attribute.data.value : false;
            s_pump_on_off_attr = on;
            ESP_LOGI(TAG, "Pump → %s", on ? "ON" : "OFF");
            pump_driver_set_power(on);
        }

    /* ── Endpoint 5: Pump speed (Level Control) ──────────────────────── */
    } else if (message->info.dst_endpoint == PUMP_ENDPOINT &&
               message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_LEVEL_CONTROL) {
        if (message->attribute.id == ESP_ZB_ZCL_ATTR_LEVEL_CONTROL_CURRENT_LEVEL_ID &&
            message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_U8) {
            uint8_t level = message->attribute.data.value
                            ? *(uint8_t *)message->attribute.data.value : 0;
            s_pump_level_attr = level;
            uint8_t pwm_level = pump_level_to_pwm(level);
            ESP_LOGI(TAG, "Pump level %u/254 -> PWM=%u", level, pwm_level);
            pump_driver_set_level(pwm_level);
        }

    }

    return ESP_OK;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id,
                                   const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((const esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    case ESP_ZB_CORE_CMD_DEFAULT_RESP_CB_ID:
        ESP_LOGD(TAG, "Received ZCL default response");
        break;
    default:
        ESP_LOGW(TAG, "Unhandled Zigbee action callback: 0x%x", callback_id);
        break;
    }
    return ret;
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  Sensor polling task                                                    */
/* ─────────────────────────────────────────────────────────────────────── */

static void sensor_poll_task(void *arg)
{
    /* Small delay so the Zigbee stack has time to finish booting */
    vTaskDelay(pdMS_TO_TICKS(5000));

    while (1) {
        for (int i = 0; i < NUM_SENSORS; i++) {
            uint16_t moisture = moisture_sensor_read(i);
            ESP_LOGI(TAG, "Sensor %d moisture: %u/10000", i + 1, moisture);

            /* Update attribute inside Zigbee stack lock so the stack can
             * safely report the new value to a bound coordinator. */
            esp_zb_lock_acquire(portMAX_DELAY);
            esp_zb_zcl_set_attribute_val(
                (uint8_t)(SENSOR_ENDPOINT_BASE + i),
                ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
                ESP_ZB_ZCL_CLUSTER_SERVER_ROLE,
                ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
                &moisture,
                false);
            report_moisture_value((uint8_t)(SENSOR_ENDPOINT_BASE + i));
            esp_zb_lock_release();
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_INTERVAL_MS));
    }
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  Endpoint builders                                                      */
/* ─────────────────────────────────────────────────────────────────────── */

static void add_moisture_endpoint(esp_zb_ep_list_t *ep_list, uint8_t ep_id)
{
    /* Cluster list ─────────────────────────────────────────────────────── */
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(
        cluster_list, create_basic_cluster(),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_identify_cluster_cfg_t identify_cfg = { .identify_time = 0 };
    esp_zb_cluster_list_add_identify_cluster(
        cluster_list, esp_zb_identify_cluster_create(&identify_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    /* Relative Humidity Measurement cluster (0x0405) used as soil moisture.
     * Same wire format: 0-10000 = 0-100%.  Home Assistant maps this to a
     * moisture sensor when the endpoint description says "soil". */
    esp_zb_humidity_meas_cluster_cfg_t moisture_cfg = {
        .measured_value = 0,
        .min_value      = 0,
        .max_value      = 10000,
    };
    esp_zb_cluster_list_add_humidity_meas_cluster(
        cluster_list, esp_zb_humidity_meas_cluster_create(&moisture_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    /* Endpoint config ──────────────────────────────────────────────────── */
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint        = ep_id,
        .app_profile_id  = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id   = ESP_ZB_HA_SIMPLE_SENSOR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_cfg);
}

static void add_pump_endpoint(esp_zb_ep_list_t *ep_list, uint8_t ep_id)
{
    /* Cluster list ─────────────────────────────────────────────────────── */
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(
        cluster_list, create_basic_cluster(),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_on_off_cluster_cfg_t on_off_cfg = { .on_off = s_pump_on_off_attr };
    esp_zb_cluster_list_add_on_off_cluster(
        cluster_list, esp_zb_on_off_cluster_create(&on_off_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_level_cluster_cfg_t level_cfg = { .current_level = s_pump_level_attr };
    esp_zb_cluster_list_add_level_cluster(
        cluster_list, esp_zb_level_cluster_create(&level_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    /* Endpoint config ──────────────────────────────────────────────────── */
    esp_zb_endpoint_config_t ep_cfg = {
        .endpoint        = ep_id,
        .app_profile_id  = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id   = ESP_ZB_HA_LEVEL_CONTROLLABLE_OUTPUT_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list, ep_cfg);
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  Zigbee task – stack init + endpoint registration                      */
/* ─────────────────────────────────────────────────────────────────────── */

static void esp_zb_task(void *pvParameters)
{
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();

    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        add_moisture_endpoint(ep_list, (uint8_t)(SENSOR_ENDPOINT_BASE + i));
    }
    add_pump_endpoint(ep_list, PUMP_ENDPOINT);

    esp_zb_device_register(ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);

    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();   /* never returns */
}

/* ─────────────────────────────────────────────────────────────────────── */
/*  app_main                                                               */
/* ─────────────────────────────────────────────────────────────────────── */

void app_main(void)
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config  = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));

    ESP_ERROR_CHECK(moisture_sensor_init());
    ESP_ERROR_CHECK(pump_driver_init());

    xTaskCreate(button_task, "button_task", 2048, NULL, 6, &s_button_task_handle);
    ESP_ERROR_CHECK(boot_button_init());

    /* Zigbee stack task (priority 5, 4 kB stack – same as reference) */
    xTaskCreate(esp_zb_task,      "Zigbee_main",  4096, NULL, 5, NULL);
    /* Sensor polling task (priority 4, runs every SENSOR_POLL_INTERVAL_MS) */
    xTaskCreate(sensor_poll_task, "sensor_poll",  4096, NULL, 4, NULL);
}
