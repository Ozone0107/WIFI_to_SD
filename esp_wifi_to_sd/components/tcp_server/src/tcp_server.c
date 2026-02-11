#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

// 引入 sd_writer 組件 (確保 SD 卡已掛載)
#include "sd_writer.h" 
// 引入自己的標頭檔
#include "tcp_server.h"

static const char *TAG = "TCP_SERVER";
#define PORT 3333

// 確保完整接收指定長度的資料 (TCP 必備)
static int recv_exact(int sock, void *buf, size_t len) {
    size_t received = 0;
    while (received < len) {
        int ret = recv(sock, (char *)buf + received, len - received, 0);
        if (ret <= 0) {
            return ret; // 錯誤或連線關閉
        }
        received += ret;
    }
    return received;
}

void tcp_server_task(void *pvParameters) {
    char rx_buffer[4096]; // 4KB 接收緩衝區
    int addr_family = AF_INET;
    int ip_protocol = IPPROTO_IP;
    struct sockaddr_in dest_addr;

    // 設定 TCP 監聽
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "TCP Server listening on port %d", PORT);

    while (1) {
        ESP_LOGI(TAG, "Waiting for connection...");
        struct sockaddr_in source_addr;
        socklen_t addr_len = sizeof(source_addr);
        
        // 等待連線
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Accepted connection");

        // 連線成功後，開始循環接收檔案
        while (1) {
            // 1. 讀取 Header (5 bytes)
            uint8_t header[5];
            int len = recv_exact(sock, header, 5);
            
            if (len <= 0) {
                ESP_LOGW(TAG, "Connection closed by client");
                break; // 斷線重連
            }

            uint8_t packet_type = header[0];
            uint32_t file_size = ntohl(*(uint32_t *)(header + 1)); // 轉 Endian

            ESP_LOGI(TAG, "Header -> Type: 0x%02X, Size: %lu bytes", packet_type, file_size);

            // 2. 決定檔名 (依照 sd_writer 的掛載點 /sdcard)
            char file_path[64];
            if (packet_type == 0x01) {
                snprintf(file_path, sizeof(file_path), "/sdcard/control.dat");
            } else if (packet_type == 0x02) {
                snprintf(file_path, sizeof(file_path), "/sdcard/frame.dat");
            } else {
                ESP_LOGE(TAG, "Unknown packet type: 0x%02X, skipping...", packet_type);
                close(sock);
                break;
            }

            // 3. 開啟檔案 (直接使用標準 IO)
            FILE *f = fopen(file_path, "wb");
            if (f == NULL) {
                ESP_LOGE(TAG, "Failed to open file: %s", file_path);
                close(sock);
                break;
            }

            // 4. 接收內容並寫入
            size_t remaining = file_size;
            size_t total_written = 0;
            
            while (remaining > 0) {
                size_t to_read = (remaining < sizeof(rx_buffer)) ? remaining : sizeof(rx_buffer);
                
                int n = recv_exact(sock, rx_buffer, to_read);
                if (n <= 0) {
                    ESP_LOGE(TAG, "Error receiving body");
                    fclose(f);
                    goto socket_error;
                }

                // 寫入 SD 卡 (這裡會自動等到 SD 卡寫完才繼續，實現流量控制)
                size_t written = fwrite(rx_buffer, 1, n, f);
                total_written += written;
                remaining -= written;
            }

            fclose(f);
            ESP_LOGI(TAG, "Saved %s (%d bytes)", file_path, total_written);
            
            // 檔案傳完，繼續迴圈等待下一個檔案...
        }

        socket_error:
        if (sock != -1) {
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}