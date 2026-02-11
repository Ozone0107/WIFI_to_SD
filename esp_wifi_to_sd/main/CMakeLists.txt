idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES sd_writer tcp_server nvs_flash esp_netif protocol_examples_common freertos)