#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_core.h>

#define ESP_RMAKER_DEF_HUMIDITY_NAME    "Humidity"
#define ESP_RMAKER_DEF_MEAS_INT_NAME    "Set measurement interval"
#define ESP_RMAKER_DEF_HIGH_TEMP_NAME   "Set high temperature"
#define ESP_RMAKER_DEF_LOW_TEMP_NAME    "Set low temperature"
#define ESP_RMAKER_DEF_HIGH_HUMID_NAME  "Set high humidity"
#define ESP_RMAKER_DEF_LOW_HUMID_NAME   "Set low humidity"

#define MIN_INTERVAL    1
#define MAX_INTERVAL    60
#define MIN_TEMP        -40
#define MAX_TEMP_SHTC3  125
#define MAX_TEMP_AM2320 80 
#define MIN_HUMID       0
#define MAX_HUMID       100

#define ESP_RMAKER_PARAM_VALUE "esp.param.value"

esp_rmaker_device_t *esp_rmaker_temp_and_humid_sensor_device_create(const char *dev_name,void *priv_data, float temperature, float humidity);