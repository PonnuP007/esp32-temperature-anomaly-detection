#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "dht.h"

#define WIFI_SSID           "Wokwi-GUEST"
#define WIFI_PASSWORD       ""
#define WIFI_CONNECTED_BIT  BIT0
#define MQTT_BROKER_URI     "mqtt://broker.hivemq.com:1883"
#define MQTT_TOPIC          "ponnu/temperature"
#define MQTT_ALERT_TOPIC    "ponnu/alerts"

static const char *TAG = "DHT";
static EventGroupHandle_t wifi_event_group;
static esp_mqtt_client_handle_t mqtt_client;

// ─── WiFi ────────────────────────────────────────────────────────────────────

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Reconnecting to WiFi...");
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                        &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);
}

// ─── MQTT ────────────────────────────────────────────────────────────────────

static void mqtt_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected to broker!");
        // Subscribe to alerts topic
        esp_mqtt_client_subscribe(mqtt_client, MQTT_ALERT_TOPIC, 0);
        ESP_LOGI(TAG, "Subscribed to alerts topic: %s", MQTT_ALERT_TOPIC);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT Message published");
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGW(TAG, "ALERT received on topic: %.*s",
                 event->topic_len, event->topic);
        ESP_LOGW(TAG, "ALERT payload: %.*s",
                 event->data_len, event->data);
        break;

    default:
        break;
    }
}

void mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
    ESP_LOGI(TAG, "MQTT client started");
}

// ─── Main ────────────────────────────────────────────────────────────────────

void app_main(void)
{
    ESP_LOGI(TAG, "Temperature Anomaly Detection System Starting...");

    wifi_init();
    ESP_LOGI(TAG, "WiFi Connected!");

    mqtt_init();
    vTaskDelay(pdMS_TO_TICKS(2000));

    float temperature = 0;
    float humidity = 0;
    char payload[64];

    while (1)
    {
        esp_err_t result = dht_read_float_data(DHT_TYPE_DHT22, GPIO_NUM_4,
                                               &humidity, &temperature);
        if (result == ESP_OK)
        {
            ESP_LOGI(TAG, "Temperature: %.1f C  Humidity: %.1f%%",
                     temperature, humidity);

            snprintf(payload, sizeof(payload),
                     "{\"temperature\": %.1f, \"humidity\": %.1f}",
                     temperature, humidity);

            esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC,
                                    payload, 0, 1, 0);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read sensor");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}