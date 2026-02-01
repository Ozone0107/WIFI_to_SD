#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

// UDP Streamer 
typedef struct {
    int port;               // UDP Port
    size_t buffer_size;     // Ring Buffer Size
} udp_streamer_config_t;

/**
 * @brief Initialize UDP Streamer
 * * @param config Configuration parameters
 * @return esp_err_t
 */
esp_err_t udp_streamer_init(udp_streamer_config_t *config);

/**
 * @brief Get Ring Buffer Handle
 * * 
 * @return RingbufHandle_t 
 */
RingbufHandle_t udp_streamer_get_buffer_handle(void);