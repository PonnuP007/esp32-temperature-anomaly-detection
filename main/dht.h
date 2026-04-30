#pragma once
#include "driver/gpio.h"
#include "esp_err.h"

typedef enum {
    DHT_TYPE_DHT11 = 11,
    DHT_TYPE_DHT22 = 22,
} dht_sensor_type_t;

esp_err_t dht_read_float_data(dht_sensor_type_t sensor_type, gpio_num_t pin, float *humidity, float *temperature);