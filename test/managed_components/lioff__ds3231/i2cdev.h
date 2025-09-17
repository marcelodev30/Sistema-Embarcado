#include "driver/i2c.h"

#define I2C_FREQ_HZ 400000
#define I2CDEV_TIMEOUT 1000

typedef struct {
    i2c_port_t port;    // I2C port number
    uint8_t addr;       // I2C address
    int sda_io_num;     // GPIO number for I2C sda signal
    int scl_io_num;     // GPIO number for I2C scl signal
    uint32_t clk_speed; // I2C clock frequency for master mode
} i2c_dev_t;

extern esp_err_t i2c_master_init(i2c_port_t port, int sda, int scl);
extern esp_err_t i2c_dev_read(const i2c_dev_t *dev, uint8_t *out_data, size_t out_size, void *in_data, size_t in_size);
extern esp_err_t i2c_dev_write(const i2c_dev_t *dev, uint8_t *out_reg, size_t out_reg_size, uint8_t *out_data, size_t out_size);
extern esp_err_t i2c_dev_read_reg(const i2c_dev_t *dev, uint8_t reg, uint8_t *in_data, size_t in_size);
extern esp_err_t i2c_dev_write_reg(const i2c_dev_t *dev, uint8_t reg, uint8_t *out_data, size_t out_size);