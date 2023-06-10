#include <driver/i2c.h>

#define SHTC3_SENSOR_ADDRESS            0x70    /* SHTC3 I2C device address */
#define SHTC3_SLEEP                     0xB098  /* sleep command */
#define SHTC3_WAKEUP                    0x3517  /* wake-up command */
#define SHTC3_MEAS_T_RH_CLOCKSTR_EN     0x7CA2  /* clock stretching enabled, read temperature first */
#define SHTC3_MEAS_RH_T_CLOCKSTR_EN     0x5C24  /* clock stretching enabled, read humidity first */
#define SHTC3_MEAS_T_RH_CLOCKSTR_DIS    0x7866  /* clock stretching disabled, read temperature first */
#define SHTC3_MEAS_RH_T_CLOCKSTR_DIS    0x58E0  /* clock stretching disabled, read humidity first */
#define SHTC3_MEAS                      CONFIG_SHTC3_MODE   /* operation mode of SHTC3 */
#define SHTC3_SOFT_RESET                0x805D  /* soft reset command */
#define SHTC3_READ_ID                   0xEFC8  /* read-out command of ID register */

#define SHTC3_CRC_POLYNOMIAL  0x31

esp_err_t SHTC3_get_temp_and_humid(i2c_port_t i2c_num, float *temp, float *humid);