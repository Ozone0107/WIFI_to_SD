#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "nvs_flash.h"

#include "readframe.h"
#include "tcp_client.h"

static const char* TAG = "APP";
static bool frame_sys_ready = false;

static void app_task(void* arg) {
    ESP_LOGI(TAG, "app_task start, HWM=%u", uxTaskGetStackHighWaterMark(NULL));

    esp_err_t sd_err = frame_system_init("0:/control.dat", "0:/frame.dat");
    ESP_LOGI(TAG, "frame_system_init=%s", esp_err_to_name(sd_err));
    ESP_LOGI(TAG, "HWM after frame_system_init=%u", uxTaskGetStackHighWaterMark(NULL));

    vTaskDelay(pdMS_TO_TICKS(1000));

    if(sd_err != ESP_OK) {
        ESP_LOGE(TAG, "frame system init failed, halt");
        //vTaskDelay(portMAX_DELAY);
        frame_sys_ready = false;
    } else {
        frame_sys_ready = true;
    }

    nvs_flash_init();
    tcp_client_start_update_task();

    vTaskDelete(NULL);
}

extern "C" void app_main(void) {
    xTaskCreate(app_task, "app_task", 16384, NULL, 5, NULL);
}