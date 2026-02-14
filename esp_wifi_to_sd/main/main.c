#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"

// 引入我們改名後的組件
#include "sd_writer.h"
#include "tcp_server.h"
 

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    

    // 1. 連接 Wi-Fi (原本的功能)
    ESP_ERROR_CHECK(example_connect());
    esp_wifi_set_ps(WIFI_PS_NONE);

    // 2. 初始化 SD 卡 (sd_writer 組件)
    // 這會把 SD 卡掛載到 /sdcard
   // sd init
    ESP_ERROR_CHECK(sd_writer_init("0:/test.bin"));  // init test file

    // 3. 啟動 TCP Server
    xTaskCreate(tcp_server_task, "tcp_server", 8192, NULL, 5, NULL);
}