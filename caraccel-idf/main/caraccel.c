#include "driver/gpio.h"

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "hal/spi_types.h"
#include "soc/uart_pins.h"

#include "mpu9250.h"
#include "screen.h"

#include <stdio.h>

#include "esp_lcd_ili9341.h"
// #include "esp_lcd_ili9341_init_cmds_2.h"
//  #include "esp_lcd_ili9341_init_cmds_2.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "gps.hpp"

static const int SDAPin = 13, SCLPin = 12;
i2c_master_dev_handle_t dev_handle;

const float accel_scale = 8192.0; // for MPU6050_RANGE_4_G

#define I2C_MASTER_SCL_IO SCLPin  /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO SDAPin  /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

#define TAG "CARACCEL"

/*static const ili9341_lcd_init_cmd_t ili9341_lcd_init_vendor[] = {
    {0xC8, (uint8_t []){0xFF, 0x93, 0x42}, 3, 0},
    {0xC0, (uint8_t []){0x0E, 0x0E}, 2, 0},
    {0xC5, (uint8_t []){0xD0}, 1, 0},
    {0xC1, (uint8_t []){0x02}, 1, 0},
    {0xB4, (uint8_t []){0x02}, 1, 0},
    {0xE0, (uint8_t []){0x00, 0x03, 0x08, 0x06, 0x13, 0x09, 0x39, 0x39, 0x48, 0x02, 0x0a, 0x08, 0x17, 0x17, 0x0F}, 15, 0},
    {0xE1, (uint8_t []){0x00, 0x28, 0x29, 0x01, 0x0d, 0x03, 0x3f, 0x33, 0x52, 0x04, 0x0f, 0x0e, 0x37, 0x38, 0x0F}, 15, 0},

    {0xB1, (uint8_t []){00, 0x1B}, 2, 0},
    {0x36, (uint8_t []){0x08}, 1, 0},
    {0x3A, (uint8_t []){0x55}, 1, 0},
    {0xB7, (uint8_t []){0x06}, 1, 0},

    {0x11, (uint8_t []){0}, 0x80, 0},
    {0x29, (uint8_t []){0}, 0x80, 0},

    {0, (uint8_t []){0}, 0xff, 0},
};*/

esp_lcd_panel_handle_t panel_handle = NULL;
lv_obj_t *label = NULL;
lv_obj_t *label2 = NULL;
lv_obj_t *label3 = NULL;

const int uart_buffer_size = (1024);
QueueHandle_t uart_queue;
static const int RXPin = 5, TXPin = 4;
const char *disable_gsv = "$PUBX,40,GSV,1,0,0,0,0,0*58\r\n";
const char *disable_gga = "$PUBX,40,GGA,1,0,0,0,0,0*5B\r\n";
const char *disable_gsa = "$PUBX,40,GSA,1,0,0,0,0,0*4F\r\n";
const char *disable_gll = "$PUBX,40,GLL,1,0,0,0,0,0*5D\r\n";
const char *cfg_115200 = "$PUBX,41,1,0007,0003,115200,0*18\r\n";
const unsigned char ubxRate10Hz[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 100, 0x00, 0x01, 0x00, 0x01, 0x00};

static const uint32_t GPSBaud = 9600;

static void event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED)
    {
        ESP_LOGI("LVGL", "Button clicked!");
        lv_label_set_text(obj, "Clicked!");
    }
}
const uart_port_t uart_num = UART_NUM_1;

SemaphoreHandle_t mtx_gps_data;
struct gps_data_t gps_data;

SemaphoreHandle_t mtx_imu_data;
struct accel_data_t accel_data;

struct accel_data_t max_accel_p;
struct accel_data_t max_accel_n;

bool must_update_max_accel = false;
bool must_update_gps_valid = false;

bool update_max_min_accel(struct accel_data_t *current, struct accel_data_t *max_p, struct accel_data_t *max_n)
{
    bool updated = false;
    if (current->accel_x > max_p->accel_x)
    {
        max_p->accel_x = current->accel_x;
        updated = true;
    }
    if (current->accel_x < max_n->accel_x)
    {
        max_n->accel_x = current->accel_x;
        updated = true;
    }
    if (current->accel_y > max_p->accel_y)
    {
        max_p->accel_y = current->accel_y;
        updated = true;
    }
    if (current->accel_y < max_n->accel_y)
    {
        max_n->accel_y = current->accel_y;
        updated = true;
    }
    if (current->accel_z > max_p->accel_z)
    {
        max_p->accel_z = current->accel_z;
        updated = true;
    }
    if (current->accel_z < max_n->accel_z)
    {
        max_n->accel_z = current->accel_z;
        updated = true;
    }

    return updated;
}

void imu_task(void *pvParameters)
{
    struct accel_data_t local_accel_data;
    // MPU6050_RANGE_4_G -> accel_scale = 8192;
    while (1)
    {
        read_accel(&dev_handle, &local_accel_data);
        // ESP_LOGI(TAG, "Accel: x=%d, y=%d, z=%d", local_accel_data.accel_x, local_accel_data.accel_y, local_accel_data.accel_z);
        if (xSemaphoreTake(mtx_imu_data, (TickType_t)10) == pdTRUE)
        {
            accel_data = local_accel_data;

            // Update max accelerations
            if (update_max_min_accel(&accel_data, &max_accel_p, &max_accel_n))
            {
                must_update_max_accel = true;
            }
            xSemaphoreGive(mtx_imu_data);
        }
        taskYIELD();
    }
}

void update_ui_task(void *pvParameters)
{
    char speed_str[16];

    while (1)
    {

        if (xSemaphoreTake(mtx_gps_data, (TickType_t)10) == pdTRUE)
        {

            lvgl_port_lock(0);
            set_lbl_gps_time(gps_data.timestr);
            // Round speed to integer
            int speed = (int)(gps_data.speed_kph + 0.5);
            snprintf(speed_str, sizeof(speed_str), "%d", speed);
            if (must_update_gps_valid)
            {
                set_lbl_speed(speed_str);
                set_lbl_gps_valid(gps_data.data_validity == 'A' || gps_data.data_validity == 'D');
                must_update_gps_valid = false;
            }

            lvgl_port_unlock();
            xSemaphoreGive(mtx_gps_data);
        }

        if (xSemaphoreTake(mtx_imu_data, (TickType_t)10) == pdTRUE)
        {

            lvgl_port_lock(0);

            float acc_x = accel_data.accel_x / accel_scale;
            float acc_y = accel_data.accel_y / accel_scale;
            float acc_z = accel_data.accel_z / accel_scale;

            set_lbl_current_accelerations(acc_x, acc_y, acc_z);

            if (must_update_max_accel)
            {
                // Update max accel labels
                acc_x = max_accel_p.accel_x / accel_scale;
                acc_y = max_accel_p.accel_y / accel_scale;
                acc_z = max_accel_p.accel_z / accel_scale;
                set_lbl_max_accelerations(acc_x, acc_y, acc_z);
                acc_x = max_accel_n.accel_x / accel_scale;
                acc_y = max_accel_n.accel_y / accel_scale;
                acc_z = max_accel_n.accel_z / accel_scale;
                set_lbl_min_accelerations(acc_x, acc_y, acc_z);

                must_update_max_accel = false;
            }

            lvgl_port_unlock();
            xSemaphoreGive(mtx_imu_data);
        }

        // vTaskDelay(100 / portTICK_PERIOD_MS);
        taskYIELD();
    }
}

void gps_read_task(void *pvParameters)
{

    struct gps_data_t internal_gps_data;
    // Read data from UART.

    uint8_t data[512];
    int length = 0;
    while (1)
    {
        ESP_LOGI(TAG, "Reading...");
        ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t *)&length));
        length = uart_read_bytes(uart_num, data, length, 100);
        ESP_LOGI(TAG, "Read %d bytes from GPS: %.*s", length, length, data);
        bool initFound = false;

        int init_idx = 0;
        for (int i = 0; i < length; i++)
        {
            if (data[i] == '$')
            {
                // Serial.println("Init of sentence");
                initFound = true;

                init_idx = i;
            }

            if (data[i] == '\n')
            {
                data[i] = 0;
                if (initFound)
                {
                    parse_nmea_sentence((const char *)(data + init_idx), &internal_gps_data);
                    if (xSemaphoreTake(mtx_gps_data, (TickType_t)10) == pdTRUE)
                    {
                        if (gps_data.data_validity != internal_gps_data.data_validity)
                        {
                            must_update_gps_valid = true;
                        }
                        gps_data = internal_gps_data;
                        xSemaphoreGive(mtx_gps_data);
                    }
                }
                initFound = false;
            }
        }

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup_imu()
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = SCLPin,
        .sda_io_num = SDAPin,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
        .flags.allow_pd = false,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    uint8_t buf[1] = {MPU6050_WHO_AM_I};
    uint8_t buffer[1] = {0};
    i2c_master_transmit_receive(dev_handle, buf, sizeof(buf), buffer, sizeof(buffer), -1);
    ESP_LOGI(TAG, "MPU9250 WHO_AM_I: 0x%02x", buffer[0]);
    init_mpu9250(&dev_handle);
}

void setup_uart()
{
    ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size * 2, 0, 0, NULL, 0));
    const uart_config_t uart_config = {
        .baud_rate = GPSBaud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, TXPin, RXPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "Disabling unnecessary NMEA sentences...");
    // Disable sentences
    uart_write_bytes(uart_num, disable_gsv, strlen(disable_gsv));
    // wait 200ms
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_num, disable_gga, strlen(disable_gga));
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_num, disable_gsa, strlen(disable_gsa));
    vTaskDelay(200 / portTICK_PERIOD_MS);
    uart_write_bytes(uart_num, disable_gll, strlen(disable_gll));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Switching GPS baud rate to 115200...");
    // switch o 115200
    uart_write_bytes(uart_num, cfg_115200, strlen(cfg_115200));
    vTaskDelay(200 / portTICK_PERIOD_MS);
    // reconfigure uart to 115200
    const uart_config_t uart_config_115200 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config_115200));

    // Set GPS update rate to 10Hz
    uart_write_bytes(uart_num, (const char *)ubxRate10Hz, sizeof(ubxRate10Hz));
}

TaskHandle_t xHandle = NULL;
TaskHandle_t uiUpdateHandle = NULL;
TaskHandle_t imuHandle = NULL;

void app_main(void)
{
    mtx_gps_data = xSemaphoreCreateMutex();
    mtx_imu_data = xSemaphoreCreateMutex();
    ESP_LOGI(TAG, "Initialize UART");
    setup_uart();
    ESP_LOGI(TAG, "Initialize IMU");
    setup_imu();

    static uint8_t ucParameterToPass;

    ESP_LOGI(TAG, "Create GPS read task");
    xTaskCreate(gps_read_task, "GPS Read Task", 2048, &ucParameterToPass, tskIDLE_PRIORITY, &xHandle);
    configASSERT(xHandle);

    ESP_LOGI(TAG, "Create IMU task");
    xTaskCreate(imu_task, "IMU Task", 2048, &ucParameterToPass, tskIDLE_PRIORITY, &imuHandle);
    configASSERT(imuHandle);

    init_ili9341(panel_handle);

    ESP_LOGI(TAG, "Create Hello World label");

    setup_ui();
    // // When touching LVGL from your app task, lock/unlock the port:
    // lvgl_port_lock(0);
    // label = lv_label_create(lv_screen_active());
    // lv_label_set_text(label, "Hello world!");
    // lv_obj_center(label);

    // lv_obj_t *btn1 = lv_button_create(lv_screen_active());
    // lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    // lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
    // lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);

    // label2 = lv_label_create(btn1);
    // lv_label_set_text(label2, "Button");
    // lv_obj_center(label2);

    // label3 = lv_label_create(lv_screen_active());
    // lv_label_set_text(label3, "Accel: x=0, y=0, z=0");
    // lv_obj_align(label3, LV_ALIGN_CENTER, 0, 40);

    // lvgl_port_unlock();

    ESP_LOGI(TAG, "Create UI update task");
    xTaskCreate(update_ui_task, "UI Update Task", 2048, &ucParameterToPass, tskIDLE_PRIORITY, &uiUpdateHandle);
    configASSERT(uiUpdateHandle);
    ESP_LOGI(TAG, "Done. LVGL task runs in background. Nothing to do in app_main.");

    /*while (1)
    {

        uint16_t row_line = EXAMPLE_LCD_V_RES / TEST_LCD_BIT_PER_PIXEL;
        uint8_t byte_per_pixel = TEST_LCD_BIT_PER_PIXEL / 8;
        uint8_t *color = (uint8_t *)heap_caps_calloc(1, row_line * EXAMPLE_LCD_H_RES * byte_per_pixel, MALLOC_CAP_DMA);

        for (int j = 0; j < TEST_LCD_BIT_PER_PIXEL; j++)
        {
            for (int i = 0; i < row_line * EXAMPLE_LCD_H_RES; i++)
            {
                for (int k = 0; k < byte_per_pixel; k++)
                {
                    color[i * byte_per_pixel + k] = (SPI_SWAP_DATA_TX(BIT(j), TEST_LCD_BIT_PER_PIXEL) >> (k * 8)) & 0xff;
                }
            }
            esp_lcd_panel_draw_bitmap(panel_handle, 0, j * row_line, EXAMPLE_LCD_H_RES, (j + 1) * row_line, color);
        }
        free(color);
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "Hello world!");
    }*/
}
