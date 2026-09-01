#include "board.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static const char *TAG = "hello_world";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT    BIT1

static EventGroupHandle_t wifi_event_group;
static int connection_attempts;
static bool wifi_is_stopping;
static esp_ip4_addr_t station_ip;

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static esp_err_t hello_page_handler(httpd_req_t *request)
{
    static const char response[] =
        "<!doctype html>"
        "<html lang=\"en\"><head><meta charset=\"utf-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>ESP32-C6</title></head>"
        "<body><h1>Hello, world!</h1><p>Served by your ESP32-C6.</p></body></html>";

    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
}

static void start_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    const httpd_uri_t hello_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = hello_page_handler,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &hello_uri));
}

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_is_stopping) {
            return;
        }
        if (connection_attempts < CONFIG_WIFI_TEST_MAXIMUM_RETRY) {
            connection_attempts++;
            ESP_LOGW(TAG,
                     "Wi-Fi disconnected; retrying (%d/%d)",
                     connection_attempts,
                     CONFIG_WIFI_TEST_MAXIMUM_RETRY);
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;
        station_ip = event->ip_info.ip;
        ESP_LOGI(TAG, "Connected. IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Hello from %s", BOARD_NAME);
    ESP_LOGI(TAG,
             "Default I2C wiring: SDA GPIO%d, SCL GPIO%d",
             BOARD_I2C_SDA_GPIO,
             BOARD_I2C_SCL_GPIO);

    init_nvs();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Scanning for nearby Wi-Fi access points...");
    wifi_scan_config_t scan_config = {
        .show_hidden = true,
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    uint16_t ap_count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Found %u access point%s", ap_count, ap_count == 1 ? "" : "s");

    if (ap_count > 0) {
        wifi_ap_record_t *access_points = calloc(ap_count, sizeof(*access_points));
        if (access_points == NULL) {
            ESP_LOGE(TAG, "Not enough memory to read scan results");
        } else {
            uint16_t records_to_read = ap_count;
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&records_to_read, access_points));
            for (uint16_t i = 0; i < records_to_read; ++i) {
                ESP_LOGI(TAG,
                         "  %2u: SSID: %-32s RSSI: %d dBm  auth mode: %d",
                         i + 1,
                         (const char *)access_points[i].ssid,
                         access_points[i].rssi,
                         access_points[i].authmode);
            }
            free(access_points);
        }
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_LOGI(TAG, "Wi-Fi scan test complete");

    if (CONFIG_WIFI_TEST_SSID[0] == '\0') {
        ESP_LOGW(TAG, "No Wi-Fi network is configured; skipping connection test");
        ESP_LOGI(TAG, "Run build.ps1 with -Menuconfig to enter the Wi-Fi test SSID and password");
        return;
    }

    wifi_event_group = xEventGroupCreate();
    configASSERT(wifi_event_group != NULL);

    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         &wifi_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                         IP_EVENT_STA_GOT_IP,
                                                         &wifi_event_handler,
                                                         NULL,
                                                         &ip_event_instance));

    wifi_config_t station_config = { 0 };
    snprintf((char *)station_config.sta.ssid,
             sizeof(station_config.sta.ssid),
             "%s",
             CONFIG_WIFI_TEST_SSID);
    snprintf((char *)station_config.sta.password,
             sizeof(station_config.sta.password),
             "%s",
             CONFIG_WIFI_TEST_PASSWORD);

    connection_attempts = 0;
    wifi_is_stopping = false;
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to configured Wi-Fi network: %s", CONFIG_WIFI_TEST_SSID);
    EventBits_t result = xEventGroupWaitBits(wifi_event_group,
                                             WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                             pdFALSE,
                                             pdFALSE,
                                             portMAX_DELAY);
    if (result & WIFI_FAILED_BIT) {
        ESP_LOGE(TAG, "Could not connect after %d attempts", CONFIG_WIFI_TEST_MAXIMUM_RETRY);
        wifi_is_stopping = true;
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,
                                                               IP_EVENT_STA_GOT_IP,
                                                               ip_event_instance));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                                                               ESP_EVENT_ANY_ID,
                                                               wifi_event_instance));
        ESP_ERROR_CHECK(esp_wifi_stop());
        ESP_ERROR_CHECK(esp_wifi_deinit());
        vEventGroupDelete(wifi_event_group);
        return;
    }

    start_web_server();
    ESP_LOGI(TAG, "Open http://" IPSTR "/ in a browser", IP2STR(&station_ip));
}
