#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <string.h>

#include "rmaker_device.h"

esp_rmaker_param_t *esp_rmaker_dropdown_param_create(const char *param_name, int val, int min, int max) {
    esp_rmaker_param_t *param = esp_rmaker_param_create(param_name, ESP_RMAKER_PARAM_VALUE,
            esp_rmaker_int(val), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_DROPDOWN);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(min), esp_rmaker_int(max), esp_rmaker_int(1));
    }
    return param;
}

/* Create device for ESP RainMaker app  */
esp_rmaker_device_t *esp_rmaker_temp_and_humid_sensor_device_create(const char *dev_name,void *priv_data, float temperature, float humidity) {
    esp_rmaker_device_t *device = esp_rmaker_device_create(dev_name, ESP_RMAKER_DEVICE_TEMP_SENSOR, priv_data);
    if (device) {
        esp_rmaker_device_add_param(device, esp_rmaker_name_param_create(ESP_RMAKER_DEF_NAME_PARAM, dev_name));
        esp_rmaker_device_add_param(device, esp_rmaker_temperature_param_create(ESP_RMAKER_DEF_TEMPERATURE_NAME, temperature));
        esp_rmaker_device_add_param(device, esp_rmaker_temperature_param_create(ESP_RMAKER_DEF_HUMIDITY_NAME, humidity));
        esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_MEAS_INT_NAME, MIN_INTERVAL, MIN_INTERVAL, MAX_INTERVAL));
        if (strcmp(dev_name, "SHTC3") == 0) {
            esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_LOW_TEMP_NAME, MIN_TEMP, MIN_TEMP, MAX_TEMP_SHTC3));
            esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_HIGH_TEMP_NAME, MAX_TEMP_SHTC3, MIN_TEMP, MAX_TEMP_SHTC3));
        } else if (strcmp(dev_name, "AM2320") == 0) {
            esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_LOW_TEMP_NAME, MIN_TEMP, MIN_TEMP, MAX_TEMP_AM2320));
            esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_HIGH_TEMP_NAME, MAX_TEMP_AM2320, MIN_TEMP, MAX_TEMP_AM2320));
        }
        esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_LOW_HUMID_NAME, MIN_HUMID, MIN_HUMID, MAX_HUMID));
        esp_rmaker_device_add_param(device, esp_rmaker_dropdown_param_create(ESP_RMAKER_DEF_HIGH_HUMID_NAME, MAX_HUMID, MIN_HUMID, MAX_HUMID));
    }
    return device;
}