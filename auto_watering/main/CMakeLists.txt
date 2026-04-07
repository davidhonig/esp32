cmake_minimum_required(VERSION 3.16)

idf_component_register(
    SRCS "auto_watering_main.c" "moisture_sensor.c" "pump_driver.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash ieee802154 esp_adc driver
)
