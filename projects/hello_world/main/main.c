#include "board.h"
#include "esp_log.h"

static const char *TAG = "hello_world";

void app_main(void)
{
    ESP_LOGI(TAG, "Hello from %s", BOARD_NAME);
    ESP_LOGI(TAG,
             "Default I2C wiring: SDA GPIO%d, SCL GPIO%d",
             BOARD_I2C_SDA_GPIO,
             BOARD_I2C_SCL_GPIO);
}

