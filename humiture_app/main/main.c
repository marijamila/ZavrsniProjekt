#include <stdio.h>
#include <sdkconfig.h>
#include <esp_log.h>
#include <string.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_scenes.h>
#include <app_wifi.h>
#include <esp_wifi.h>
#include <esp_pm.h>
#include <esp_event.h>
#include <app_insights.h>

#include "rmaker_device.h"
#include "app_priv.h"

#define DEFAULT_LISTEN_INTERVAL CONFIG_WIFI_LISTEN_INTERVAL
#define DEFAULT_BEACON_TIMEOUT  CONFIG_WIFI_BEACON_TIMEOUT

#if CONFIG_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_POWER_SAVE_NONE
#define PS_MODE WIFI_PS_NONE
#else
#define PS_MODE WIFI_PS_NONE
#endif /*CONFIG_POWER_SAVE_MODEM*/

static const char *TAG = "main";

esp_rmaker_device_t *SHTC3;
esp_rmaker_device_t *AM2320;

/* Callback function for SHTC3 sensor */
static esp_err_t SHTC3_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx) {
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, ESP_RMAKER_DEF_MEAS_INT_NAME) == 0) {
        SHTC3_set_measure_int(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_LOW_TEMP_NAME) == 0) {
        SHTC3_set_low_temp(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HIGH_TEMP_NAME) == 0) {
        SHTC3_set_high_temp(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_LOW_HUMID_NAME) == 0) {
        SHTC3_set_low_humid(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HIGH_HUMID_NAME) == 0) {
        SHTC3_set_high_humid(val.val.i);
    }

    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

/* Callback function for AM2320 sensor */
static esp_err_t AM2320_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
                                 const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx) {
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }
    const char *param_name = esp_rmaker_param_get_name(param);
    if (strcmp(param_name, ESP_RMAKER_DEF_MEAS_INT_NAME) == 0) {
        AM2320_set_measure_int(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_LOW_TEMP_NAME) == 0) {
        AM2320_set_low_temp(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HIGH_TEMP_NAME) == 0) {
        AM2320_set_high_temp(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_LOW_HUMID_NAME) == 0) {
        AM2320_set_low_humid(val.val.i);
    } else if (strcmp(param_name, ESP_RMAKER_DEF_HIGH_HUMID_NAME) == 0) {
        AM2320_set_high_humid(val.val.i);
    }

    esp_rmaker_param_update_and_report(param, val);
    return ESP_OK;
}

void app_main(void) {

    /* Initialize NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* Set power management configuration. Requires CONFIG_FREERTOS_USE_TICKLESS_IDLE=y */
#if CONFIG_IDF_TARGET_ESP32
    esp_pm_config_esp32_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S2
    esp_pm_config_esp32s2_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32S3
    esp_pm_config_esp32s3_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C3
    esp_pm_config_esp32c3_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C2
    esp_pm_config_esp32c2_t pm_config = {
#elif CONFIG_IDF_TARGET_ESP32C6
    esp_pm_config_esp32c6_t pm_config = {
#endif
        .max_freq_mhz = CONFIG_MAX_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_MIN_CPU_FREQ_MHZ,
        .light_sleep_enable = true
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_config));

    /* Initialize Wi-Fi */
    app_wifi_init();
    
    /* Initialize the ESP RainMaker agent */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Temperature and Humidity Sensor");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }

    /* Create devices for the app */
    SHTC3 = esp_rmaker_temp_and_humid_sensor_device_create("SHTC3", NULL, SHTC3_get_current_temperature(), SHTC3_get_current_humidity());
    AM2320 = esp_rmaker_temp_and_humid_sensor_device_create("AM2320", NULL, AM2320_get_current_temperature(), AM2320_get_current_humidity());
    esp_rmaker_device_add_cb(SHTC3, SHTC3_write_cb, NULL);
    esp_rmaker_device_add_cb(AM2320, AM2320_write_cb, NULL);
    esp_rmaker_node_add_device(node, SHTC3);
    esp_rmaker_node_add_device(node, AM2320);

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling */
    esp_rmaker_schedule_enable();

    /* Enable scenes */
    esp_rmaker_scenes_enable();

    /* Enable insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    app_insights_enable();

    /* Start the ESP RainMaker agent */
    esp_rmaker_start();

    /* Wi-Fi power save */
    wifi_config_t wifi_config;
    wifi_config.sta.listen_interval = DEFAULT_LISTEN_INTERVAL;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_inactive_time(WIFI_IF_STA, DEFAULT_BEACON_TIMEOUT));

    esp_wifi_set_ps(DEFAULT_PS_MODE);

    /* Start the Wi-Fi */
    err = app_wifi_start(POP_TYPE_RANDOM);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start Wifi. Aborting!!!");
        vTaskDelay(5000/portTICK_PERIOD_MS);
        abort();
    }
    
    /* Initialize I2C master and tasks for measuring */
    app_task_init();
}