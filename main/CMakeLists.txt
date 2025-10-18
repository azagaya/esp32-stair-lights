idf_component_register(SRCS "mqtt.c" "hello_world_main.c"
                       PRIV_REQUIRES spi_flash esp_wifi nvs_flash mqtt
                       INCLUDE_DIRS "")
