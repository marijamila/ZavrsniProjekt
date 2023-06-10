#include <driver/i2c.h>

#define AM2320_SENSOR_ADDRESS       0xB8    /* AM2320 I2C device address */
#define READING_REGISTER_DATA       0x03    /* read one or more data registers */
#define WRITE_MULTIPLE_REGISTERS    0x10    /* multiple sets of binary data to write multiple registers */
#define START_ADDRESS               0x00    /* address of high humidity; start address for reading measurements */
#define NUMBER_OF_REGISTERS         0x04    /* high humidity - 0x00, low humidity - 0x01,
                                               high temperature - 0x02, low temperature - 0x03 */

#define AM2320_CRC_POLYNOMIAL  0xA001

esp_err_t AM2320_get_temp_and_humid(i2c_port_t i2c_num, float *temp, float *humid);