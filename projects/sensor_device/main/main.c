#include "board.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TAG = "sensor_device";

#define SHT41_ADDRESS             0x44
#define VEML7700_ADDRESS          0x10
#define OLED_ADDRESS_PRIMARY      0x3C
#define OLED_ADDRESS_ALTERNATE    0x3D
#define WIFI_CONNECTED_BIT        BIT0
#define WIFI_FAILED_BIT           BIT1
#define SENSOR_UPDATE_PERIOD_MS   2000
#define WIFI_RETRY_DELAY_MS       (3U * 60U * 1000U)
#define MQTT_QOS                  1
#define OLED_SSID_MAX_CHARS       8

static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t sht41_device;
static i2c_master_dev_handle_t veml7700_device;
static i2c_master_dev_handle_t oled_device;
static esp_mqtt_client_handle_t mqtt_client;
static EventGroupHandle_t wifi_event_group;
static int connection_attempts;
static bool wifi_is_stopping;
static esp_ip4_addr_t station_ip;
static float latest_temperature_f;
static float latest_humidity_percent;
static float latest_lux;
static bool readings_valid;
static bool light_reading_valid;
static bool mqtt_connected;
static bool time_sync_started;
static float latest_battery_percent;
static bool battery_reading_valid;

#if BOARD_HAS_BATTERY_MONITOR && CONFIG_SENSOR_BATTERY_MONITOR
static adc_oneshot_unit_handle_t battery_adc;
static bool battery_adc_ready;
#endif

#define MQTT_BASE_TOPIC           "home/" CONFIG_SENSOR_MQTT_DEVICE_ID
#define MQTT_STATE_TOPIC          MQTT_BASE_TOPIC "/state"
#define MQTT_AVAILABILITY_TOPIC   MQTT_BASE_TOPIC "/availability"

static void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static const char *known_i2c_device(uint8_t address)
{
    switch (address) {
    case VEML7700_ADDRESS:
        return "VEML7700 ambient-light sensor";
    case OLED_ADDRESS_PRIMARY:
    case OLED_ADDRESS_ALTERNATE:
        return "SSD1306 OLED display";
    case SHT41_ADDRESS:
        return "SHT41 temperature/humidity sensor";
    default:
        return "unknown device";
    }
}

static bool i2c_device_present(uint8_t address)
{
    return i2c_master_probe(i2c_bus, address, 20) == ESP_OK;
}

static void init_i2c(void)
{
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = BOARD_I2C_SDA_GPIO,
        .scl_io_num = BOARD_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Scanning I2C: SDA GPIO%d, SCL GPIO%d", BOARD_I2C_SDA_GPIO,
             BOARD_I2C_SCL_GPIO);
    uint8_t count = 0;
    for (uint8_t address = 0x08; address <= 0x77; ++address) {
        if (i2c_device_present(address)) {
            ESP_LOGI(TAG, "I2C device 0x%02X: %s", address, known_i2c_device(address));
            count++;
        }
    }
    ESP_LOGI(TAG, "I2C scan complete: %u device%s found", count, count == 1 ? "" : "s");
}

static i2c_master_dev_handle_t add_i2c_device(uint8_t address)
{
    i2c_master_dev_handle_t device;
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &config, &device));
    return device;
}

static bool sht41_crc_matches(const uint8_t *data)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < 2; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc == data[2];
}

static esp_err_t read_sht41(float *temperature_c, float *humidity_percent)
{
    const uint8_t measure_high_precision = 0xFD;
    uint8_t data[6];
    ESP_RETURN_ON_ERROR(i2c_master_transmit(sht41_device, &measure_high_precision, 1, 100), TAG,
                        "SHT41 measurement command failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(i2c_master_receive(sht41_device, data, sizeof(data), 100), TAG,
                        "SHT41 read failed");
    if (!sht41_crc_matches(&data[0]) || !sht41_crc_matches(&data[3])) {
        ESP_LOGE(TAG, "SHT41 returned data with an invalid CRC");
        return ESP_ERR_INVALID_CRC;
    }

    const uint16_t raw_temperature = ((uint16_t)data[0] << 8) | data[1];
    const uint16_t raw_humidity = ((uint16_t)data[3] << 8) | data[4];
    *temperature_c = -45.0f + (175.0f * raw_temperature / 65535.0f);
    *humidity_percent = -6.0f + (125.0f * raw_humidity / 65535.0f);
    if (*humidity_percent < 0.0f) {
        *humidity_percent = 0.0f;
    } else if (*humidity_percent > 100.0f) {
        *humidity_percent = 100.0f;
    }
    return ESP_OK;
}

static void init_veml7700(void)
{
    /* ALS gain x1, 100 ms integration, interrupts disabled, ALS powered on. */
    const uint8_t configuration[] = {0x00, 0x00, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(veml7700_device, configuration, sizeof(configuration), 100));
    vTaskDelay(pdMS_TO_TICKS(110));
}

static esp_err_t read_veml7700(float *lux)
{
    const uint8_t als_data_register = 0x04;
    uint8_t data[2];
    ESP_RETURN_ON_ERROR(i2c_master_transmit_receive(veml7700_device,
                                                     &als_data_register,
                                                     1,
                                                     data,
                                                     sizeof(data),
                                                     100),
                        TAG,
                        "VEML7700 read failed");
    const uint16_t raw_als = ((uint16_t)data[1] << 8) | data[0];
    /* VEML7700 x1 gain with 100 ms integration: 0.0576 lux per count. */
    *lux = raw_als * 0.0576f;
    return ESP_OK;
}

#if BOARD_HAS_BATTERY_MONITOR && CONFIG_SENSOR_BATTERY_MONITOR
static void init_battery_monitor(void)
{
    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &battery_adc));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(battery_adc, ADC_CHANNEL_0, &channel_config));
    battery_adc_ready = true;
}

static void read_battery_percent(void)
{
    if (!battery_adc_ready) {
        return;
    }

    int raw = 0;
    if (adc_oneshot_read(battery_adc, ADC_CHANNEL_0, &raw) != ESP_OK) {
        return;
    }

    /* A0 is fed by the documented 1:2 divider: 200k from BAT and 200k to GND. */
    const float battery_voltage = ((float)raw * 3.3f / 4095.0f) * 2.0f;
    float percent = (battery_voltage - 3.30f) * (100.0f / 0.90f);
    if (percent < 0.0f) {
        percent = 0.0f;
    } else if (percent > 100.0f) {
        percent = 100.0f;
    }
    latest_battery_percent = percent;
    battery_reading_valid = true;
}
#endif

static const uint8_t *glyph_for(char character)
{
    static const uint8_t blank[] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t digits[][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    };
    static const uint8_t letters[][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x03, 0x04, 0x78, 0x04, 0x03}, {0x61, 0x51, 0x49, 0x45, 0x43},
    };
    static const uint8_t colon[] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t decimal[] = {0x00, 0x60, 0x60, 0x00, 0x00};
    static const uint8_t percent[] = {0x63, 0x13, 0x08, 0x64, 0x63};
    static const uint8_t minus[] = {0x08, 0x08, 0x08, 0x08, 0x08};

    if (character >= '0' && character <= '9') {
        return digits[character - '0'];
    }
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    if (character >= 'A' && character <= 'Z') {
        return letters[character - 'A'];
    }
    switch (character) {
    case ':': return colon;
    case '.': return decimal;
    case '%': return percent;
    case '-': return minus;
    default: return blank;
    }
}

static void format_status_line(char *status_line, size_t status_line_size)
{
    char time_text[] = "--:--";
    time_t now;
    struct tm local_time;
    time(&now);
    localtime_r(&now, &local_time);
    if (local_time.tm_year >= (2020 - 1900)) {
        strftime(time_text, sizeof(time_text), "%H:%M", &local_time);
    }

    wifi_ap_record_t access_point = {0};
    if (esp_wifi_sta_get_ap_info(&access_point) == ESP_OK) {
        char ssid[OLED_SSID_MAX_CHARS + 1];
        snprintf(ssid, sizeof(ssid), "%.*s", OLED_SSID_MAX_CHARS, (char *)access_point.ssid);
        snprintf(status_line, status_line_size, "%s %s %d", time_text, ssid, access_point.rssi);
    } else {
        snprintf(status_line, status_line_size, "%s OFFLINE", time_text);
    }
}

static void oled_draw_text(uint8_t *framebuffer, uint8_t page, const char *text)
{
    uint8_t column = 0;
    while (*text != '\0' && column + 5 < 128) {
        const uint8_t *glyph = glyph_for(*text++);
        for (uint8_t i = 0; i < 5; ++i) {
            framebuffer[page * 128 + column++] = glyph[i];
        }
        framebuffer[page * 128 + column++] = 0x00;
    }
}

static void oled_write_framebuffer(const uint8_t *framebuffer)
{
    uint8_t transaction[129];
    transaction[0] = 0x40;
    for (uint8_t page = 0; page < 8; ++page) {
        transaction[0] = 0x00;
        transaction[1] = (uint8_t)(0xB0 + page);
        transaction[2] = 0x00;
        transaction[3] = 0x10;
        ESP_ERROR_CHECK(i2c_master_transmit(oled_device, transaction, 4, 100));
        transaction[0] = 0x40;
        memcpy(&transaction[1], &framebuffer[page * 128], 128);
        ESP_ERROR_CHECK(i2c_master_transmit(oled_device, transaction, sizeof(transaction), 100));
    }
}

static void init_oled(void)
{
    const uint8_t commands[] = {
        0x00, 0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14,
        0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB,
        0x40, 0xA4, 0xA6, 0xAF,
    };
    ESP_ERROR_CHECK(i2c_master_transmit(oled_device, commands, sizeof(commands), 100));
}

static void render_readings(float temperature_f, float humidity_percent, float lux)
{
    uint8_t framebuffer[128 * 8] = {0};
    char status_line[22];
    char temperature_line[22];
    char humidity_line[22];
    char lux_line[22];
    char battery_line[22];
    format_status_line(status_line, sizeof(status_line));
    snprintf(temperature_line, sizeof(temperature_line), "TEMP: %.1f F", temperature_f);
    snprintf(humidity_line, sizeof(humidity_line), "HUM : %.1f%%", humidity_percent);
    snprintf(lux_line, sizeof(lux_line), "LUX : %.1f", lux);
    if (battery_reading_valid) {
        snprintf(battery_line, sizeof(battery_line), "BAT : %.0f%%", latest_battery_percent);
    } else {
        snprintf(battery_line, sizeof(battery_line), "BAT : --");
    }
    oled_draw_text(framebuffer, 0, status_line);
    oled_draw_text(framebuffer, 2, temperature_line);
    oled_draw_text(framebuffer, 4, humidity_line);
    oled_draw_text(framebuffer, 6, lux_line);
    oled_draw_text(framebuffer, 7, battery_line);
    oled_write_framebuffer(framebuffer);
}

static void start_time_synchronization(void)
{
    if (time_sync_started) {
        return;
    }

    /* Central time with US daylight-saving rules. */
    setenv("TZ", "CST6CDT,M3.2.0/2,M11.1.0/2", 1);
    tzset();
    esp_sntp_config_t time_config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&time_config);
    time_sync_started = true;
    ESP_LOGI(TAG, "Synchronizing time with pool.ntp.org");
}

static void mqtt_publish_discovery_config(const char *object_id,
                                          const char *name,
                                          const char *value_template,
                                          const char *device_class,
                                          const char *unit)
{
    char topic[192];
    char payload[1024];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/config",
             CONFIG_SENSOR_MQTT_DEVICE_ID, object_id);
    snprintf(payload, sizeof(payload),
             "{\"name\":\"%s\",\"unique_id\":\"%s_%s\","
             "\"state_topic\":\"%s\",\"value_template\":\"%s\","
             "\"device_class\":\"%s\",\"state_class\":\"measurement\","
             "\"unit_of_measurement\":\"%s\","
             "\"availability_topic\":\"%s\","
             "\"payload_available\":\"online\",\"payload_not_available\":\"offline\","
             "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\","
             "\"manufacturer\":\"Espressif/Seeed Studio\",\"model\":\"%s\"}}",
             name, CONFIG_SENSOR_MQTT_DEVICE_ID, object_id, MQTT_STATE_TOPIC,
             value_template, device_class, unit, MQTT_AVAILABILITY_TOPIC,
             CONFIG_SENSOR_MQTT_DEVICE_ID, CONFIG_SENSOR_MQTT_DEVICE_NAME, BOARD_NAME);
    esp_mqtt_client_publish(mqtt_client, topic, payload, 0, MQTT_QOS, true);
}

static void mqtt_publish_home_assistant_discovery(void)
{
    mqtt_publish_discovery_config("temperature", "Temperature",
                                  "{{ value_json.temperature_f }}", "temperature", "\\u00b0F");
    mqtt_publish_discovery_config("humidity", "Humidity",
                                  "{{ value_json.humidity }}", "humidity", "%");
    mqtt_publish_discovery_config("illuminance", "Ambient Light",
                                  "{{ value_json.illuminance }}", "illuminance", "lx");
}

static void mqtt_publish_readings(void)
{
    if (!mqtt_connected || !readings_valid || !light_reading_valid) {
        return;
    }

    char payload[160];
    snprintf(payload, sizeof(payload),
             "{\"temperature_f\":%.1f,\"humidity\":%.1f,\"illuminance\":%.1f}",
             latest_temperature_f, latest_humidity_percent, latest_lux);
    esp_mqtt_client_publish(mqtt_client, MQTT_STATE_TOPIC, payload, 0, MQTT_QOS, true);
}

static void mqtt_event_handler(void *argument, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)argument;
    (void)event_base;
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        mqtt_connected = true;
        ESP_LOGI(TAG, "Connected to MQTT broker");
        mqtt_publish_home_assistant_discovery();
        esp_mqtt_client_publish(event->client, MQTT_AVAILABILITY_TOPIC, "online", 0,
                                MQTT_QOS, true);
        mqtt_publish_readings();
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_connected = false;
        ESP_LOGW(TAG, "Disconnected from MQTT broker; reconnecting");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT connection error");
        break;
    default:
        break;
    }
}

static void start_mqtt_client(void)
{
    if (CONFIG_SENSOR_MQTT_PASSWORD[0] == '\0') {
        ESP_LOGW(TAG, "MQTT password is blank; configure Home Assistant MQTT settings in menuconfig");
        return;
    }

    const esp_mqtt_client_config_t config = {
        .broker.address.uri = CONFIG_SENSOR_MQTT_BROKER_URI,
        .credentials.username = CONFIG_SENSOR_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_SENSOR_MQTT_PASSWORD,
        .credentials.client_id = CONFIG_SENSOR_MQTT_DEVICE_ID,
        .session.last_will = {
            .topic = MQTT_AVAILABILITY_TOPIC,
            .msg = "offline",
            .msg_len = 0,
            .qos = MQTT_QOS,
            .retain = true,
        },
        .network.reconnect_timeout_ms = 10000,
    };

    mqtt_client = esp_mqtt_client_init(&config);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                                    mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
}

static void sensor_task(void *argument)
{
    uint32_t elapsed_ms = 0;
    while (true) {
        float temperature_c;
        float humidity_percent;
        if (read_sht41(&temperature_c, &humidity_percent) == ESP_OK) {
            latest_temperature_f = (temperature_c * 9.0f / 5.0f) + 32.0f;
            latest_humidity_percent = humidity_percent;
            readings_valid = true;
        }
        if (veml7700_device != NULL && read_veml7700(&latest_lux) == ESP_OK) {
            light_reading_valid = true;
        }
#if BOARD_HAS_BATTERY_MONITOR && CONFIG_SENSOR_BATTERY_MONITOR
        read_battery_percent();
#endif
        if (oled_device != NULL && readings_valid && light_reading_valid) {
            render_readings(latest_temperature_f, latest_humidity_percent, latest_lux);
        }
        elapsed_ms += SENSOR_UPDATE_PERIOD_MS;
        if (elapsed_ms >= CONFIG_SENSOR_MQTT_PUBLISH_INTERVAL_SECONDS * 1000U) {
            mqtt_publish_readings();
            elapsed_ms = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_UPDATE_PERIOD_MS));
    }
}

static esp_err_t sensor_page_handler(httpd_req_t *request)
{
    char response[700];
    if (readings_valid) {
        snprintf(response, sizeof(response),
                 "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
                 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
                 "<meta http-equiv=\"refresh\" content=\"5\"><title>ESP32-C6 Sensor</title></head>"
                 "<body><h1>ESP32-C6 Sensor Device</h1><p>Temperature: <strong>%.1f &deg;F</strong></p>"
                 "<p>Humidity: <strong>%.1f %%</strong></p><p>Ambient light: <strong>%.1f lux</strong></p>"
                 "<p>Refreshes every 5 seconds.</p></body></html>",
                 latest_temperature_f, latest_humidity_percent, latest_lux);
    } else {
        snprintf(response, sizeof(response),
                 "<!doctype html><html><body><h1>ESP32-C6 Sensor Device</h1>"
                 "<p>Waiting for a valid SHT41 reading.</p></body></html>");
    }
    httpd_resp_set_type(request, "text/html");
    return httpd_resp_send(request, response, HTTPD_RESP_USE_STRLEN);
}

static void start_web_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(httpd_start(&server, &config));
    const httpd_uri_t page_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = sensor_page_handler,
        .user_ctx = NULL,
    };
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &page_uri));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_is_stopping) {
            return;
        }
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        if (connection_attempts < CONFIG_WIFI_TEST_MAXIMUM_RETRY) {
            connection_attempts++;
            ESP_LOGW(TAG, "Wi-Fi disconnected; retrying (%d/%d)", connection_attempts,
                     CONFIG_WIFI_TEST_MAXIMUM_RETRY);
            ESP_ERROR_CHECK(esp_wifi_connect());
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAILED_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;
        station_ip = event->ip_info.ip;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void connect_wifi_and_start_server(void)
{
    if (CONFIG_WIFI_TEST_SSID[0] == '\0') {
        ESP_LOGW(TAG, "No Wi-Fi network configured; run menuconfig to enter credentials");
        return;
    }

    wifi_event_group = xEventGroupCreate();
    configASSERT(wifi_event_group != NULL);
    esp_event_handler_instance_t wifi_event_instance;
    esp_event_handler_instance_t ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                         wifi_event_handler, NULL,
                                                         &wifi_event_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                         wifi_event_handler, NULL,
                                                         &ip_event_instance));

    wifi_config_t station_config = {0};
    snprintf((char *)station_config.sta.ssid, sizeof(station_config.sta.ssid), "%s",
             CONFIG_WIFI_TEST_SSID);
    snprintf((char *)station_config.sta.password, sizeof(station_config.sta.password), "%s",
             CONFIG_WIFI_TEST_PASSWORD);
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    connection_attempts = 0;
    wifi_is_stopping = false;
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &station_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    bool services_started = false;
    while (true) {
        EventBits_t result = xEventGroupWaitBits(wifi_event_group,
                                                 WIFI_CONNECTED_BIT | WIFI_FAILED_BIT,
                                                 pdTRUE, pdFALSE, portMAX_DELAY);
        if (result & WIFI_CONNECTED_BIT) {
            connection_attempts = 0;
            if (!services_started) {
                start_web_server();
                ESP_LOGI(TAG, "Open http://" IPSTR "/ in a browser", IP2STR(&station_ip));
                start_time_synchronization();
                start_mqtt_client();
                services_started = true;
            }
        }
        if (result & WIFI_FAILED_BIT) {
            ESP_LOGW(TAG, "Wi-Fi offline; sensor display continues. Retrying in 3 minutes");
            connection_attempts = 0;
            vTaskDelay(pdMS_TO_TICKS(WIFI_RETRY_DELAY_MS));
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting sensor device on %s", BOARD_NAME);
    init_nvs();
    init_i2c();

    if (!i2c_device_present(SHT41_ADDRESS)) {
        ESP_LOGE(TAG, "SHT41 not found at 0x44; sensor readings will not start");
    } else {
        sht41_device = add_i2c_device(SHT41_ADDRESS);
    }

    if (!i2c_device_present(VEML7700_ADDRESS)) {
        ESP_LOGE(TAG, "VEML7700 not found at 0x10; light readings will not start");
    } else {
        veml7700_device = add_i2c_device(VEML7700_ADDRESS);
        init_veml7700();
    }

    const uint8_t oled_address = i2c_device_present(OLED_ADDRESS_PRIMARY)
                                     ? OLED_ADDRESS_PRIMARY
                                     : OLED_ADDRESS_ALTERNATE;
    if (!i2c_device_present(oled_address)) {
        ESP_LOGE(TAG, "SSD1306 OLED not found at 0x3C or 0x3D");
    } else {
        oled_device = add_i2c_device(oled_address);
        init_oled();
    }

#if BOARD_HAS_BATTERY_MONITOR && CONFIG_SENSOR_BATTERY_MONITOR
    init_battery_monitor();
#endif

    if (sht41_device != NULL) {
        xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    connect_wifi_and_start_server();
}
