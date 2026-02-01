idf_component_register(SRCS "udp_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES  udp_streamer nvs_flash esp_netif protocol_examples_common udp_streamer freertos esp_ringbuf)
