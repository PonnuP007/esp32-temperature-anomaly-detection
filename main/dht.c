#include "dht.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

static const char *TAG = "DHT";

static esp_err_t dht_read_raw(gpio_num_t pin, uint8_t data[5])
{
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(pin, 1);
    esp_rom_delay_us(30);
    gpio_set_direction(pin, GPIO_MODE_INPUT);

    int timeout = 10000;
    while (gpio_get_level(pin) == 1) if (--timeout == 0) return ESP_ERR_TIMEOUT;
    timeout = 10000;
    while (gpio_get_level(pin) == 0) if (--timeout == 0) return ESP_ERR_TIMEOUT;
    timeout = 10000;
    while (gpio_get_level(pin) == 1) if (--timeout == 0) return ESP_ERR_TIMEOUT;

    for (int i = 0; i < 40; i++) {
        timeout = 10000;
        while (gpio_get_level(pin) == 0) if (--timeout == 0) return ESP_ERR_TIMEOUT;
        esp_rom_delay_us(35);
        data[i / 8] <<= 1;
        if (gpio_get_level(pin) == 1) data[i / 8] |= 1;
        timeout = 10000;
        while (gpio_get_level(pin) == 1) if (--timeout == 0) return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature)
{
    uint8_t data[5] = {0};
    esp_err_t ret = dht_read_raw(pin, data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor read failed");
        return ret;
    }

    if (sensor_type == DHT_TYPE_DHT22) {
        *humidity    = ((data[0] << 8) | data[1]) / 10.0f;
        *temperature = (((data[2] & 0x7F) << 8) | data[3]) / 10.0f;
        if (data[2] & 0x80) *temperature = -*temperature;
    } else {
        *humidity    = data[0];
        *temperature = data[2];
    }
    return ESP_OK;
}