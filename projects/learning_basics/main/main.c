#include "board.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

void app_main(void)
{
    unsigned int count = 0;
    ESP_LOGI("learning", "Starting on %s", BOARD_NAME);

    while (1)
    {
        ESP_LOGI("learning", "Learning build 2: count=%u", count++);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_LEARNING_INTERVAL_MS));
    }
}
