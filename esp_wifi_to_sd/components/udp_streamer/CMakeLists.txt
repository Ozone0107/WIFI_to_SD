idf_component_register(
    SRCS  "src/udp_streamer.c"

    INCLUDE_DIRS "include"

    REQUIRES lwip freertos esp_common esp_system esp_ringbuf
)