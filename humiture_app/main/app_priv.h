#pragma once
#include <stdint.h>
#include <stdbool.h>

extern esp_rmaker_device_t *SHTC3;
extern esp_rmaker_device_t *AM2320;

void app_task_init();
float SHTC3_get_current_temperature();
float SHTC3_get_current_humidity();
float AM2320_get_current_temperature();
float AM2320_get_current_humidity();
void SHTC3_set_measure_int(uint8_t val);
void SHTC3_set_low_temp(int8_t val);
void SHTC3_set_high_temp(int8_t val);
void SHTC3_set_low_humid(uint8_t val);
void SHTC3_set_high_humid(uint8_t val);
void AM2320_set_measure_int(uint8_t val);
void AM2320_set_low_temp(int8_t val);
void AM2320_set_high_temp(int8_t val);
void AM2320_set_low_humid(uint8_t val);
void AM2320_set_high_humid(uint8_t val);