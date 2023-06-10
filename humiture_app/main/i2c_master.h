#include <driver/i2c.h>

#define I2C_MASTER_SCL_IO         CONFIG_I2C_MASTER_SCL         /* gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO         CONFIG_I2C_MASTER_SDA         /* gpio number for I2C master data  */
#define I2C_MASTER_NUM            CONFIG_I2C_MASTER_PORT_NUM    /* I2C port number for master device */
#define I2C_MASTER_FREQ_HZ        100000                        /* I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                             /* I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                             /* I2C master doesn't need buffer */

#define WRITE_BIT       I2C_MASTER_WRITE /* I2C master write */
#define READ_BIT        I2C_MASTER_READ  /* I2C master read */
#define ACK_CHECK_EN    0x1              /* I2C master will check ack from slave */
#define ACK_CHECK_DIS   0x0              /* I2C master will not check ack from slave */
#define ACK_VAL         0x0              /* I2C ack value */
#define NACK_VAL        0x1              /* I2C nack value */

#define ERR_TEMP_CRC    -2
#define ERR_HUMID_CRC   -3
#define ERR_CRC         -4

esp_err_t i2c_master_init();