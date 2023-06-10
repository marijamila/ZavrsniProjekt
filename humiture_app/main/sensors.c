#include <sdkconfig.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h> 
#include <esp_rmaker_standard_params.h> 
#include <app_reset.h>
#include <string.h>
#include <esp_sleep.h>
#include <esp_timer.h>
#include <math.h>

#include "SHTC3.h"
#include "AM2320.h"
#include "i2c_master.h"
#include "app_priv.h"
#include "rmaker_device.h"

static const char *TAG1 = "SHTC3";
static const char *TAG2 = "AM2320";

// SemaphoreHandle_t i2c_mux = NULL;

esp_timer_handle_t AM2320_timer;
esp_timer_handle_t SHTC3_timer;

static float SHTC3_temperature;
static float SHTC3_humidity;
static float AM2320_temperature;
static float AM2320_humidity;

static uint8_t SHTC3_meas_int = MIN_INTERVAL;
static int8_t SHTC3_t_low = MIN_TEMP;
static int8_t SHTC3_t_high = MAX_TEMP_SHTC3;
static uint8_t SHTC3_h_low = MIN_HUMID;
static uint8_t SHTC3_h_high = MAX_HUMID;
static uint8_t AM2320_meas_int = MIN_INTERVAL;
static int8_t AM2320_t_low = MIN_TEMP;
static int8_t AM2320_t_high = MAX_TEMP_AM2320;
static uint8_t AM2320_h_low = MIN_HUMID;
static uint8_t AM2320_h_high = MAX_HUMID;

void go_to_sleep(void) {
    esp_sleep_enable_timer_wakeup(esp_timer_get_next_alarm() - esp_timer_get_time() - 10000);
    // esp_sleep_enable_timer_wakeup(2 * 60 * 1000000);
    esp_light_sleep_start();
}

/* Tasks for measuring, updating values and sending notifications */
static void SHTC3_measure(void* arg) {
    // esp_err_t err;

    // while (1) {
    //     TickType_t currTick = xTaskGetTickCount();
    //     xSemaphoreTake(i2c_mux, portMAX_DELAY);
        
    esp_err_t err = SHTC3_get_temp_and_humid(I2C_MASTER_NUM, &SHTC3_temperature, &SHTC3_humidity);
    if (err == ESP_OK) {
        ESP_LOGI(TAG1, "Temperature: %.02f °C, Humidity: %.02f %%", SHTC3_temperature, SHTC3_humidity);
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(SHTC3, ESP_RMAKER_DEF_TEMPERATURE_NAME),
                                            esp_rmaker_float(SHTC3_temperature));
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(SHTC3, ESP_RMAKER_DEF_HUMIDITY_NAME),
                                            esp_rmaker_float(SHTC3_humidity));
        char string[60];
        if (SHTC3_temperature < SHTC3_t_low) {
            sprintf(string, "SHTC3: Temperature bellow %d °C\nIt is currently %.02f °C", SHTC3_t_low, SHTC3_temperature);
            esp_rmaker_raise_alert(string);
        }
        if (SHTC3_temperature > SHTC3_t_high) {
            sprintf(string, "SHTC3: Temperature above %d °C\nIt is currently %.02f °C", SHTC3_t_high, SHTC3_temperature);
            esp_rmaker_raise_alert(string);
        }
        if (SHTC3_humidity < SHTC3_h_low) {
            sprintf(string, "SHTC3: Humidity bellow %d %%\nIt is currently %.02f %%", SHTC3_h_low, SHTC3_humidity);
            esp_rmaker_raise_alert(string);
        }
        if (SHTC3_humidity > SHTC3_h_high) {
            sprintf(string, "SHTC3: Humidity above %d %%\nIt is currently %.02f %%", SHTC3_h_high, SHTC3_humidity);
            esp_rmaker_raise_alert(string);
        }
    } else {
        if (err == ESP_FAIL) {
            ESP_LOGW(TAG1, "ESP_FAIL -> Sending command error, slave doesn't ACK the transfer.");
            esp_rmaker_raise_alert("SHTC3: sensor disconnected");
        } else if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG1, "ESP_ERR_TIMEOUT -> Operation timeout because the bus is busy.");
            esp_rmaker_raise_alert("SHTC3: error with i2c communication");
        } else if (err == ERR_TEMP_CRC) {
            ESP_LOGW(TAG1, "ERR_TEMP_CRC -> CRC temperature validation failed.");
            esp_rmaker_raise_alert("SHTC3: CRC temperature validation failed");
        } else if (err == ERR_HUMID_CRC) {
            ESP_LOGW(TAG1, "ERR_HUMID_CRC -> CRC humidity validation failed.");
            esp_rmaker_raise_alert("SHTC3: CRC humidity validation failed");
        } else if (err == ERR_CRC) {
            ESP_LOGW(TAG1, "ERR_CRC -> CRC valitation failed.");
            esp_rmaker_raise_alert("SHTC3: CRC validation failed");
        }
    }

    go_to_sleep();
        // xSemaphoreGive(i2c_mux);
    //     vTaskDelayUntil(&currTick, (SHTC3_meas_int * 60000)/2 / portTICK_PERIOD_MS);
    // }
    // vSemaphoreDelete(i2c_mux);
    // vTaskDelete(NULL);
}

static void AM2320_measure(void* arg) {
    // esp_err_t err;

    // while(1) {    
    //     TickType_t currTick = xTaskGetTickCount();
    //     xSemaphoreTake(i2c_mux, portMAX_DELAY);

    esp_err_t err = AM2320_get_temp_and_humid(I2C_MASTER_NUM, &AM2320_temperature, &AM2320_humidity);
    if (err == ESP_OK) {
        ESP_LOGI(TAG2, "Temperature: %.01f °C, Humidity: %.01f %%", AM2320_temperature, AM2320_humidity);
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(AM2320, ESP_RMAKER_DEF_TEMPERATURE_NAME),
                                           esp_rmaker_float(AM2320_temperature));
        esp_rmaker_param_update_and_report(esp_rmaker_device_get_param_by_name(AM2320, ESP_RMAKER_DEF_HUMIDITY_NAME),
                                           esp_rmaker_float(AM2320_humidity));
        char string[60];
        if (AM2320_temperature < AM2320_t_low) {
            sprintf(string, "AM2320: Temperature bellow %d °C\nIt is currently %.01f °C", AM2320_t_low, AM2320_temperature);
            esp_rmaker_raise_alert(string);
        }
        if (AM2320_temperature > AM2320_t_high) {
            sprintf(string, "AM2320: Temperature above %d °C\nIt is currently %.01f °C", AM2320_t_high, AM2320_temperature);
            esp_rmaker_raise_alert(string);
        }
        if (AM2320_humidity < AM2320_h_low) {
            sprintf(string, "AM2320: Humidity bellow %d %%\nIt is currently %.01f %%", AM2320_h_low, AM2320_humidity);
            esp_rmaker_raise_alert(string);
        }
        if (AM2320_humidity > AM2320_h_high) {
            sprintf(string, "AM2320: Humidity above %d %%\nIt is currently %.01f %%", AM2320_h_high, AM2320_humidity);
            esp_rmaker_raise_alert(string);
        }
    } else {
        if (err == ESP_FAIL) {
            ESP_LOGW(TAG2, "ESP_FAIL -> Sending command error, slave doesn't ACK the transfer.");
            esp_rmaker_raise_alert("AM2320: sensor disconnected");
        } else if (err == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG2, "ESP_ERR_TIMEOUT -> Operation timeout because the bus is busy.");
            esp_rmaker_raise_alert("AM2320: error with i2c communication");
        } else if (err == ERR_CRC) {
            ESP_LOGW(TAG2, "ERR_CRC -> CRC valitation failed.");
            esp_rmaker_raise_alert("AM2320: CRC validation failed");
        }
    }

    go_to_sleep();
    //     xSemaphoreGive(i2c_mux);
    //     vTaskDelayUntil(&currTick, (AM2320_meas_int * 6000)/2 / portTICK_PERIOD_MS);
    // }
    // vSemaphoreDelete(i2c_mux);
    // vTaskDelete(NULL);
}

/* Functions for setting variables for notifications */
void SHTC3_set_measure_int(uint8_t val) {
    SHTC3_meas_int = val;
    ESP_ERROR_CHECK(esp_timer_stop(SHTC3_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(SHTC3_timer, SHTC3_meas_int * 60000000));
    SHTC3_measure(SHTC3_timer);
}
void SHTC3_set_low_temp(int8_t val) {
    SHTC3_t_low = val;
}

void SHTC3_set_high_temp(int8_t val) {
    SHTC3_t_high = val;
}

void SHTC3_set_low_humid(uint8_t val) {
    SHTC3_h_low = val;
}

void SHTC3_set_high_humid(uint8_t val) {
    SHTC3_h_high = val;
}

void AM2320_set_measure_int(uint8_t val) {
    AM2320_meas_int = val;
    ESP_ERROR_CHECK(esp_timer_stop(AM2320_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(AM2320_timer, AM2320_meas_int * 60000000));
    AM2320_measure(AM2320_timer);
}

void AM2320_set_low_temp(int8_t val) {
    AM2320_t_low = val;
}

void AM2320_set_high_temp(int8_t val) {
    AM2320_t_high = val;
}

void AM2320_set_low_humid(uint8_t val) {
    AM2320_h_low = val;
}

void AM2320_set_high_humid(uint8_t val) {
    AM2320_h_high = val;
}

/* Get functions for the app  */
float SHTC3_get_current_temperature() {
    return SHTC3_temperature;
}

float SHTC3_get_current_humidity() {
    return SHTC3_humidity;
}

float AM2320_get_current_temperature() {
    return AM2320_temperature;
}

float AM2320_get_current_humidity() {
    return AM2320_humidity;
}

/* Create tasks for measuring temperature and humidity with different sensors */
void app_task_init() {

    ESP_ERROR_CHECK(i2c_master_init());

    const esp_timer_create_args_t SHTC3_timer_args = {
            .callback = &SHTC3_measure,
            .name = "SHTC3's timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&SHTC3_timer_args, &SHTC3_timer));

    const esp_timer_create_args_t AM2320_timer_args = {
            .callback = &AM2320_measure,
            .name = "AM2320's timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&AM2320_timer_args, &AM2320_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(SHTC3_timer, SHTC3_meas_int * 60000000));
    ESP_ERROR_CHECK(esp_timer_start_periodic(AM2320_timer, AM2320_meas_int * 60000000));


    // i2c_mux = xSemaphoreCreateMutex();

    // xTaskCreate(SHTC3_measure, "SHTC3_measure", 4096, NULL, 10, NULL);
    // xTaskCreate(AM2320_measure, "AM2320_measure", 4096, NULL, 10, NULL);
}