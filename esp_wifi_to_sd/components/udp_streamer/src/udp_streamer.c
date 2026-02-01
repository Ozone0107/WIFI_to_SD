#include "udp_streamer.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "freertos/task.h"

static const char *TAG = "udp_streamer";
static RingbufHandle_t buf_handle = NULL;
static int g_port = 3333; // Default port

static void udp_server_task(void *pvParameters)
{
    char rx_buffer[128];
    // Bind address structure
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(g_port);

    while (1) {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Set socket receive timeout
        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "UDP Server listening on port %d", g_port);

        struct sockaddr_storage source_addr;
        socklen_t socklen = sizeof(source_addr);

        while (1) {
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);

            if (len < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                    break; // Exit inner loop to restart socket
                }
                continue;
            }
            else {
                // Successfully received data
                if (buf_handle != NULL) {
                    UBaseType_t res = xRingbufferSend(buf_handle, rx_buffer, len, pdMS_TO_TICKS(10));
                    if (res != pdTRUE) {
                        ESP_LOGE(TAG, "Ring Buffer Full! Packet Dropped.");
                    }
                }
            }
        }

        if (sock != -1) {
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

// Initialize UDP Streamer
esp_err_t udp_streamer_init(udp_streamer_config_t *config)
{
    g_port = config->port;

    // Create Ring Buffer
    buf_handle = xRingbufferCreate(config->buffer_size, RINGBUF_TYPE_NOSPLIT);
    if (buf_handle == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return ESP_FAIL;
    }

    // Create UDP Server Task
    xTaskCreate(udp_server_task, "udp_server", 4096, NULL, 5, NULL);
    
    return ESP_OK;
}

// Get Ring Buffer Handle
RingbufHandle_t udp_streamer_get_buffer_handle(void)
{
    return buf_handle;
}