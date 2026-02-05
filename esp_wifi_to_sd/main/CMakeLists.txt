idf_component_register(SRCS "udp_server.c"
                    INCLUDE_DIRS "."
                    REQUIRES  sd_writer udp_streamer nvs_flash esp_netif protocol_examples_common udp_streamer freertos esp_ringbuf)
