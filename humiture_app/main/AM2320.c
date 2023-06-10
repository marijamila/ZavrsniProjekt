#include <stdbool.h>
#include <sdkconfig.h>
#include <driver/i2c.h>

#include "AM2320.h"
#include "i2c_master.h"

static uint8_t read[6];
static uint8_t CRC_lsb, CRC_msb;
static uint16_t raw_temp, raw_humid;

/*
 * 1. send wakeup command
 * ________________________________________________________________________________
 * | start | slave_addr + wr_bit + nack | sensor waits for at least 800 us | stop |
 * |-------|----------------------------|----------------------------------|------|
 */
esp_err_t AM2320_wakeup(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2320_SENSOR_ADDRESS | WRITE_BIT, ACK_CHECK_DIS);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/*
 * 1. set measurement command
 * ________________________________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | read_reg_data + ack | start_addr + ack | num_of_reg + ack | stop |
 * |-------|---------------------------|---------------------|------------------|------------------|------|
 */
esp_err_t AM2320_start_measurement(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2320_SENSOR_ADDRESS | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, READING_REGISTER_DATA, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, START_ADDRESS, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, NUMBER_OF_REGISTERS, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}

/*
 * 1. read data
 * _______________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read read_reg_data + ack | read num_of_reg + ack | read humid_msb + ack |
 * |-------|---------------------------|--------------------------|-----------------------|----------------------|
 * ______________________________________________________________________________________________________________________
 * | read humid_lsb + ack | read temp_msb + ack | read temp_lsb + ack | read CRC_lsb + ack | read CRC_msb + nack | stop |
 * |----------------------|---------------------|---------------------|--------------------|---------------------|------|
 */
esp_err_t AM2320_read_data(i2c_port_t i2c_num) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, AM2320_SENSOR_ADDRESS | READ_BIT, ACK_CHECK_EN);     /* 0xB9 */
    i2c_master_read_byte(cmd, &read[0], ACK_VAL);   /* read_reg_data */
    i2c_master_read_byte(cmd, &read[1], ACK_VAL);   /* num_of_reg */
    i2c_master_read_byte(cmd, &read[2], ACK_VAL);   /* raw_h_msb */
    i2c_master_read_byte(cmd, &read[3], ACK_VAL);   /* raw_h_lsb */
    i2c_master_read_byte(cmd, &read[4], ACK_VAL);   /* raw_t_msb */
    i2c_master_read_byte(cmd, &read[5], ACK_VAL);   /* raw_t_lsb */
    i2c_master_read_byte(cmd, &CRC_lsb, ACK_VAL);
    i2c_master_read_byte(cmd, &CRC_msb, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return err;
}


/* Check if read data is valid */
bool AM2320_check_CRC(uint8_t data[], uint8_t num_of_bytes, uint16_t checksum) {
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < num_of_bytes; i++) {
        crc ^= (data[i]);
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x01) {
                crc = (crc >> 1);
                crc = crc ^ AM2320_CRC_POLYNOMIAL;
            } else {
                crc = (crc >> 1);
            }
        }
    }

    return (crc == checksum);
}

/*
 * 1. wake sensor up
 * 2. start measurement
 * 3. wait for at least 1.5 ms
 * 4. read data
 * 5. check data and calculate real values
 * 6. sensor automatically goes to sleep
 */
esp_err_t AM2320_get_temp_and_humid(i2c_port_t i2c_num, float *temp, float *humid) {  
    esp_err_t err = AM2320_wakeup(i2c_num);
    if (err != ESP_OK) return err;

    err = AM2320_start_measurement(i2c_num);
    if (err != ESP_OK) return err;

    vTaskDelay(2 / portTICK_PERIOD_MS);

    err = AM2320_read_data(i2c_num);
    if (err != ESP_OK) return err;

    uint16_t CRC = (uint16_t) ((CRC_msb << 8) | CRC_lsb);
    if (AM2320_check_CRC(read, 6, CRC)) {
        raw_temp = (uint16_t) ((read[4] << 8) | read[5]);   /* ((raw_t_msb << 8) | raw_t_lsb) */
        raw_humid = (uint16_t) ((read[2] << 8) | read[3]);  /* ((raw_h_msb << 8) | raw_h_lsb) */
        *temp = raw_temp / 10.0;
        *humid = raw_humid / 10.0;
    } else {
        return ERR_CRC;
    }

    return ESP_OK;
}