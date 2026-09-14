idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "." ".."
                    REQUIRES driver esp_driver_i2c esp_wifi esp_http_server
                             esp_netif esp_timer heat_ch422g heat_observer heat_web
                             nvs_flash spiffs)
