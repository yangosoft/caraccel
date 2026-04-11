#include "mpu9250.h"

#include "esp_log.h"

#define TAG "MPU9250"

void init_mpu9250(i2c_master_dev_handle_t *dev_handle)
{
    // MPU6050_RANGE_4_G MPU6050_BAND_21_HZ
    //   Adafruit_BusIO_Register(i2c_dev, MPU6050_SMPLRT_DIV, 1); sample_rate_div.write(divisor=0);
    uint8_t buf[2] = {MPU6050_SMPLRT_DIV, 0}; // sample rate divider
    i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000);

    // MPU6050_RANGE_4_G
    buf[0] = MPU6050_ACCEL_CONFIG;
    buf[1] = MPU6050_RANGE_4_G << 3; // accel
    i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000);

    // read back
    buf[0] = MPU6050_ACCEL_CONFIG;
    uint8_t buffer[1] = {0};
    i2c_master_transmit_receive(*dev_handle, buf, sizeof(buf), buffer, sizeof(buffer), 1000);
    ESP_LOGI(TAG, "MPU9250 ACCEL_CONFIG: 0x%02x", buffer[0]);

    buf[0] = MPU6050_CONFIG;
    buf[1] = MPU6050_BAND_21_HZ; // gyro
    i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000);

    // read back
    buf[0] = MPU6050_CONFIG;
    i2c_master_transmit_receive(*dev_handle, buf, sizeof(buf), buffer, sizeof(buffer), 1000);
    ESP_LOGI(TAG, "MPU9250 CONFIG: 0x%02x", buffer[0]);

    buf[0] = MPU6050_PWR_MGMT_1;
    buf[1] = 1; // power management
    i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000);

    // read back
    buf[0] = MPU6050_PWR_MGMT_1;
    i2c_master_transmit_receive(*dev_handle, buf, sizeof(buf), buffer,
                                sizeof(buffer), 1000);
    ESP_LOGI(TAG, "MPU9250 PWR_MGMT_1: 0x%02x", buffer[0]);
}

void read_accel(i2c_master_dev_handle_t *dev_handle, struct accel_data_t *accel_data)
{
    accel_data->accel_x = 0;
    accel_data->accel_y = 0;
    accel_data->accel_z = 0;
    uint8_t buf[1] = {MPU6050_ACCEL_OUT};
    uint8_t data[6] = {0};

    i2c_master_transmit_receive(*dev_handle, buf, sizeof(buf), data, sizeof(data), 100);

    accel_data->accel_x = (int16_t)(data[0] << 8 | data[1]);
    accel_data->accel_y = (int16_t)(data[2] << 8 | data[3]);
    accel_data->accel_z = (int16_t)(data[4] << 8 | data[5]);
}