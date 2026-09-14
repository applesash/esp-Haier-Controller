idf_component_register(
    SRCS "heat_modbus.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_uart esp_driver_gpio freertos
)
