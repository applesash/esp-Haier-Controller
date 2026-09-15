idf_component_register(
    SRCS "heat_web.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server esp_wifi esp_netif nvs_flash spiffs json heat_modbus heat_observer
)
