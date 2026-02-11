#include "sd_writer.h"

#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "ff.h"

static const char *TAG = "sd_writer";

/* ========= internal state ========= */

static sdmmc_card_t *card = NULL;
static FIL file;
static bool mounted = false;
static bool file_opened = false;

/* ========= internal helpers ========= */

static esp_err_t mount_sd(void)
{
    if (mounted) return ESP_OK;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;  
    slot_config.gpio_cd = GPIO_NUM_NC;
    slot_config.gpio_wp = GPIO_NUM_NC; 
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };
    ESP_LOGI(TAG, "before esp vfs");
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(
        "/sdcard",
        &host,
        &slot_config,
        &mount_config,
        &card
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    mounted = true;
    ESP_LOGI(TAG, "SD mounted");
    return ESP_OK;
}

/* ========= API ========= */

esp_err_t sd_writer_init(const char *file_path)
{
    ESP_ERROR_CHECK(mount_sd());

    if (file_opened) {
        ESP_LOGW(TAG, "file already opened");
        return ESP_OK;
    }

    FRESULT fr = f_open(
        &file,
        file_path,
        FA_WRITE | FA_CREATE_ALWAYS
    );

    if (fr != FR_OK) {
        ESP_LOGE(TAG, "f_open failed (%d)", fr);
        return ESP_FAIL;
    }

    file_opened = true;
    ESP_LOGI(TAG, "file opened: %s", file_path);
    return ESP_OK;
}

esp_err_t sd_writer_write(const void *data, size_t len)
{
    if (!file_opened) return ESP_ERR_INVALID_STATE;

    UINT bw = 0;
    FRESULT fr = f_write(&file, data, len, &bw);

    if (fr != FR_OK || bw != len) {
        ESP_LOGE(TAG, "f_write failed fr=%d bw=%d/%d", fr, bw, len);
        return ESP_FAIL;
    }

    return ESP_OK;
}

void sd_writer_close(void)
{
    if (file_opened) {
        f_close(&file);
        file_opened = false;
        ESP_LOGI(TAG, "file closed");
    }
}