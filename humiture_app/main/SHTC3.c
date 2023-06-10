#include <stdbool.h>
#include <sdkconfig.h>
#include <driver/i2c.h>
#include <math.h>

#include "SHTC3.h"
#include "i2c_master.h"

static uint8_t raw_t[2], raw_h[2];
static uint8_t tempCRC, humidCRC;
static uint16_t raw_temp, raw_humid;

/*
 * 1. send wakeup command
 * __________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | wakeup_msb + ack | wakeup_lsb + ack | stop |
 * |-------|---------------------------|------------------|------------------|------|
 * 2. wait for 240 us
 */
esp_err_t SHTC3_wakeup(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_WAKEUP >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_WAKEUP & 0xFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    vTaskDelay(0.24 / portTICK_PERIOD_MS);
    return err;
}

/*
 * 1. set measurement mode
 * ______________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | meas_msb + ack | meas_lsb + ack | stop |
 * |-------|---------------------------|----------------|----------------|------|
 */
esp_err_t SHTC3_start_measurement(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_MEAS_T_RH_CLOCKSTR_EN >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_MEAS_T_RH_CLOCKSTR_EN & 0xFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/*
 * Mode: clock stretching enabled, read temperature first
 * 1. read data
 * ________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read temp_msb + ack | read temp_lsb + ack | read temp_crc + ack |
 * |-------|---------------------------|---------------------|---------------------|---------------------|
 * _____________________________________________________________________________
 * | read humid_msb + ack | read humid_lsb + ack | read humid_crc + ack | stop |
 * |----------------------|----------------------|----------------------|------|
*/
esp_err_t SHTC3_measure_and_read_1(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &raw_t[0], ACK_VAL);  /* raw_t_msb */
    i2c_master_read_byte(cmd, &raw_t[1], ACK_VAL);  /* raw_t_lsb */
    i2c_master_read_byte(cmd, &tempCRC, ACK_VAL);
    i2c_master_read_byte(cmd, &raw_h[0], ACK_VAL);  /* raw_h_msb */
    i2c_master_read_byte(cmd, &raw_h[1], ACK_VAL);  /* raw_h_lsb */
    i2c_master_read_byte(cmd, &humidCRC, ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/*
 * Mode: clock stretching enabled, read humidity first
 * 1. read data
 * __________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read humid_msb + ack | read humid_lsb + ack | read humid_crc + ack |
 * |-------|---------------------------|----------------------|----------------------|----------------------|
 * __________________________________________________________________________
 * | read temp_msb + ack | read temp_lsb + ack | read temp_crc + ack | stop |
 * |---------------------|---------------------|---------------------|------|
*/
esp_err_t SHTC3_measure_and_read_2(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &raw_h[0], ACK_VAL);  /* raw_h_msb */
    i2c_master_read_byte(cmd, &raw_h[1], ACK_VAL);  /* raw_h_lsb */
    i2c_master_read_byte(cmd, &humidCRC, ACK_VAL);
    i2c_master_read_byte(cmd, &raw_t[0], ACK_VAL);  /* raw_t_msb */
    i2c_master_read_byte(cmd, &raw_t[1], ACK_VAL);  /* raw_t_lsb */
    i2c_master_read_byte(cmd, &tempCRC, ACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/*
 * Mode: clock stretching disabled, read temperature first
 * 1. read data
 * _____________________________________________
 * | start | slave_addr + rd_bit + nack | stop |
 * |-------|----------------------------|------|
 * 2. wait for end of measurement
 * _______________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read temp_msb + ack | read temp_lsb + ack | read temp_crc + ack |
 * |-------|---------------------------|---------------------|---------------------|---------------------|
 * _____________________________________________________________________________
 * | read humid_msb + ack | read humid_lsb + ack | read humid_crc + ack | stop |
 * |----------------------|----------------------|----------------------|------|
*/
esp_err_t SHTC3_measure_and_read_3(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) return err;

    do {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &raw_t[0], ACK_VAL);  /* raw_t_msb */
    i2c_master_read_byte(cmd, &raw_t[1], ACK_VAL);  /* raw_t_lsb */
    i2c_master_read_byte(cmd, &tempCRC, ACK_VAL);
    i2c_master_read_byte(cmd, &raw_h[0], ACK_VAL);  /* raw_h_msb */
    i2c_master_read_byte(cmd, &raw_h[1], ACK_VAL);  /* raw_h_lsb */
    i2c_master_read_byte(cmd, &humidCRC, ACK_VAL);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    } while (err != ESP_OK);    /* wait for end of measurement */

    return err;
}

/*
 * Mode: clock stretching disabled, read humidity first
 * 1. read data
 * _____________________________________________
 * | start | slave_addr + rd_bit + nack | stop |
 * |-------|----------------------------|------|
 * 2. wait for end of measurement
 * __________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read humid_msb + ack | read humid_lsb + ack | read humid_crc + ack |
 * |-------|---------------------------|----------------------|----------------------|----------------------|
 * __________________________________________________________________________
 * | read temp_msb + ack | read temp_lsb + ack | read temp_crc + ack | stop |
 * |---------------------|---------------------|---------------------|------|
*/
esp_err_t SHTC3_measure_and_read_4(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) return err;

    do {
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &raw_h[0], ACK_VAL);  /* raw_h_msb */
    i2c_master_read_byte(cmd, &raw_h[1], ACK_VAL);  /* raw_h_lsb */
    i2c_master_read_byte(cmd, &humidCRC, ACK_VAL);
    i2c_master_read_byte(cmd, &raw_t[0], ACK_VAL);  /* raw_t_msb */
    i2c_master_read_byte(cmd, &raw_t[1], ACK_VAL);  /* raw_t_lsb */
    i2c_master_read_byte(cmd, &tempCRC, ACK_VAL);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    } while (err != ESP_OK);    /* wait for end of measurement */

    return err;
}



/*
 * 1. send sleep command
 * ________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | sleep_msb + ack | sleep_lsb + ack | stop |
 * |-------|---------------------------|-----------------|-----------------|------|
 */
esp_err_t SHTC3_sleep(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDRESS << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_SLEEP >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, SHTC3_SLEEP & 0xFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/* Check if read data is valid */
bool SHTC3_check_CRC(uint8_t data[], uint8_t num_of_bytes, uint8_t checksum){
    uint8_t crc = 0xFF;

    for (uint8_t i = 0; i < num_of_bytes; i++) {
    crc ^= (data[i]);
        for (uint8_t bit = 8; bit > 0; --bit) {
            if(crc & 0x80) {
                crc = (crc << 1) ^ SHTC3_CRC_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return (crc == checksum);
}

/*
 * 1. wake sensor up
 * 2. start measurement
 * 3. wait for 12.1 ms
 * 4. read data
 * 5. validate data and calculate real values
 * 6. set sensor to sleep mode
 */
esp_err_t SHTC3_get_temp_and_humid(i2c_port_t i2c_num,  float *temp, float *humid) {
    esp_err_t err = SHTC3_wakeup(i2c_num);
    if (err != ESP_OK) return err;

    do {
        err = SHTC3_start_measurement(i2c_num);   /* sensor sometimes sends NACK instead of ACK here, hence do-while loop*/
    } while (err != ESP_OK);

    vTaskDelay(12.1 / portTICK_PERIOD_MS);
    
    if (SHTC3_MEAS == SHTC3_MEAS_T_RH_CLOCKSTR_EN)          err = SHTC3_measure_and_read_1(i2c_num);
    else if (SHTC3_MEAS == SHTC3_MEAS_RH_T_CLOCKSTR_EN)     err = SHTC3_measure_and_read_2(i2c_num);
    else if (SHTC3_MEAS == SHTC3_MEAS_T_RH_CLOCKSTR_DIS)    err = SHTC3_measure_and_read_3(i2c_num);
    else if (SHTC3_MEAS == SHTC3_MEAS_RH_T_CLOCKSTR_DIS)    err = SHTC3_measure_and_read_4(i2c_num);

    bool bool_temp = SHTC3_check_CRC(raw_t, 2, tempCRC);
    bool bool_humid = SHTC3_check_CRC(raw_h, 2, humidCRC);
    if (bool_temp) {
        raw_temp = (uint16_t) ((raw_t[0] << 8) | raw_t[1]); /* ((raw_t_msb << 8) | raw_t_lsb) */
        *temp = -45 + 175 * (raw_temp / pow(2, 16));
        *temp = floor(100 * *temp) / 100.0;       /* sensor has 0.01 resolution */
    } 
    if (bool_humid) {
        raw_humid = (uint16_t) ((raw_h[0] << 8) | raw_h[1]);    /* ((raw_h_msb << 8) | raw_h_lsb) */
        *humid = 100 * (raw_humid / pow(2, 16));
        *humid = floor(100 * *humid) / 100.0;     /* sensor has 0.01 resolution */
    }
    if (!bool_temp || !bool_humid) {
        SHTC3_sleep(i2c_num);
        if (!bool_temp) {
            if (!bool_humid) {
                return ERR_CRC;
            } else {
                return ERR_TEMP_CRC;
            }
        } else {
            return ERR_HUMID_CRC;
        }
    }

    return SHTC3_sleep(i2c_num);
}