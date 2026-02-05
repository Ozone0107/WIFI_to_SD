#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "udp_streamer.h" 
#include "sd_writer.h"

static const char *TAG = "APP_MAIN";

// Task to process received packets
static void packet_processing_task(void *pvParameters)
{
    // Get Ring Buffer Handle
    RingbufHandle_t buf_handle = udp_streamer_get_buffer_handle();
    
    size_t item_size;
    char *item;

    while (1) {
        // Receive item from Ring Buffer
        item = (char *)xRingbufferReceive(buf_handle, &item_size, portMAX_DELAY);

        if (item != NULL) {
            // Process the received packet
            ESP_LOGI(TAG, "Processing %d bytes...", item_size);
            // Write data to SD card here!

            esp_err_t err = sd_writer_write(item, item_size);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "SD write fail");
            }
            
            // Return item to Ring Buffer
            vRingbufferReturnItem(buf_handle, (void *)item);
        }
    }
}

void app_main(void)
{
    // Initialize NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Connect to Wi-Fi
    ESP_ERROR_CHECK(example_connect());

    // sd init
    ESP_ERROR_CHECK(sd_writer_init("0:/test.bin"));  // init test file


    // Initialize UDP Streamer
    udp_streamer_config_t stream_cfg = {
        .port = 3333,
        .buffer_size = 8192
    };
    
    if (udp_streamer_init(&stream_cfg) == ESP_OK) {
        ESP_LOGI(TAG, "UDP Streamer Initialized");
    } else {
        ESP_LOGE(TAG, "Failed to init UDP Streamer");
        return;
    }

    // Create Packet Processing Task
    xTaskCreate(packet_processing_task, "packet_proc", 4096, NULL, 5, NULL);
}