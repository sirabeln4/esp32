idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES board_support esp_adc esp_driver_i2c esp_event esp_http_server esp_netif esp_wifi mqtt nvs_flash)
