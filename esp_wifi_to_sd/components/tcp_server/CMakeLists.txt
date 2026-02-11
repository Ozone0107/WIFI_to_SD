idf_component_register(
    SRCS  "src/tcp_server.c"

    INCLUDE_DIRS "include"

    REQUIRES sd_writer lwip freertos esp_common esp_system esp_ringbuf
)