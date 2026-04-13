#include "screen.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_lcd_types.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "hal/spi_types.h"
#include "lvgl.h"
#include "soc/uart_pins.h"

#define ILI9341_TFTWIDTH 240  ///< ILI9341 max TFT width
#define ILI9341_TFTHEIGHT 320 ///< ILI9341 max TFT height
#define MADCTL_MY 0x80        ///< Bottom to top
#define MADCTL_MX 0x40        ///< Right to left
#define MADCTL_MV 0x20        ///< Reverse Mode
#define MADCTL_ML 0x10        ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00       ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08       ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04        ///< LCD refresh right to left
#define ILI9341_MADCTL 0x36   ///< Memory Access Control
const int TFT_CS = 18;
const int TFT_DC = 20;
const int TFT_MOSI = 21;
const int TFT_SLK = 22;
const int TFT_RST = 19;
const int TFT_LED = -1;
const int TFT_MISO = 23;

#define EXAMPLE_PIN_NUM_LCD_PCLK 22
#define EXAMPLE_PIN_NUM_LCD_MOSI 21
#define EXAMPLE_PIN_NUM_LCD_CS 18
#define EXAMPLE_PIN_NUM_LCD_DC 20
#define EXAMPLE_PIN_NUM_LCD_RST 19
#define EXAMPLE_LCD_HOST SPI2_HOST
#define EXAMPLE_LCD_H_RES 240
#define EXAMPLE_LCD_V_RES 320
#define TEST_LCD_BIT_PER_PIXEL 16

static const ili9341_lcd_init_cmd_t ili9341_lcd_init_vendor[] = {
    {ILI9341_MADCTL, (uint8_t[]){(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR)}, 1, 0},
    {0, (uint8_t[]){0}, 0xff, 0},
};

#define TAG "SCREEN"

lv_obj_t *lbl_speed;
lv_obj_t *lbl_0_100;
lv_obj_t *lbl_80_100;
lv_obj_t *lbl_80_120;
lv_obj_t *lbl_max_speed;
lv_obj_t *lbl_current_acc_x;
lv_obj_t *lbl_current_acc_y;
lv_obj_t *lbl_current_acc_z;
lv_obj_t *lbl_max_acc_x_p;
lv_obj_t *lbl_max_acc_y_p;
lv_obj_t *lbl_max_acc_z_p;
lv_obj_t *lbl_max_acc_x_n;
lv_obj_t *lbl_max_acc_y_n;
lv_obj_t *lbl_max_acc_z_n;
lv_obj_t *lbl_gps_time;
lv_obj_t *lbl_gps_valid;

static bool fix_w_get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter,
                                uint32_t letter_next, int16_t fixed_width)
{
    bool ret = lv_font_get_glyph_dsc_fmt_txt(font, dsc, letter, letter_next);
    if (!ret)
        return false;

    /* Set a fixed width */
    dsc->adv_w = fixed_width;
    dsc->ofs_x = (dsc->adv_w - dsc->box_w) / 2;
    return true;
}

static bool fix_w_get_glyph_dsc_14(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter,
                                   uint32_t letter_next)
{
    return fix_w_get_glyph_dsc(font, dsc, letter, letter_next, 14);
}

static bool fix_w_get_glyph_dsc_20(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter,
                                   uint32_t letter_next)
{
    return fix_w_get_glyph_dsc(font, dsc, letter, letter_next, 13);
}

static bool fix_w_get_glyph_dsc_48(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter,
                                   uint32_t letter_next)
{
    return fix_w_get_glyph_dsc(font, dsc, letter, letter_next, 30);
}

void init_ili9341(esp_lcd_panel_handle_t panel_handle)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    const spi_bus_config_t bus_config = ILI9341_PANEL_BUS_SPI_CONFIG(EXAMPLE_PIN_NUM_LCD_PCLK, EXAMPLE_PIN_NUM_LCD_MOSI,
                                                                     EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t));

    ESP_ERROR_CHECK(spi_bus_initialize(EXAMPLE_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const esp_lcd_panel_io_spi_config_t io_config = ILI9341_PANEL_IO_SPI_CONFIG(EXAMPLE_PIN_NUM_LCD_CS, EXAMPLE_PIN_NUM_LCD_DC,
                                                                                /*example_callback*/ NULL, /*example_callback_ctx*/ NULL);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ILI9341 panel driver");

    ili9341_vendor_config_t vendor_config = {
        // Uncomment these lines if use custom initialization commands
        .init_cmds = ili9341_lcd_init_vendor,
        .init_cmds_size = sizeof(ili9341_lcd_init_vendor) / sizeof(ili9341_lcd_init_cmd_t),
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,  // Set to -1 if not use
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, // RGB element order: R-G-B
        .bits_per_pixel = TEST_LCD_BIT_PER_PIXEL,   // Implemented by LCD command `3Ah` (16/18)
        //.vendor_config = &vendor_config,            // Uncomment this line if use custom initialization commands
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    // ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Init LVGL port");
    const lvgl_port_cfg_t lvgl_cfg =
        ESP_LVGL_PORT_INIT_CONFIG(); // creates LVGL task/timer for you
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    ESP_LOGI(TAG, "Add LVGL display");
    lvgl_port_rotation_cfg_t rotation = {
        .swap_xy = true,
        .mirror_x = true,
        .mirror_y = true,
    };

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * 80, // tune as needed
        .double_buffer = true,
        .vres = EXAMPLE_LCD_H_RES,
        .hres = EXAMPLE_LCD_V_RES,
        .monochrome = false,
        .rotation = rotation,
        .color_format = LV_COLOR_FORMAT_RGB565, // keep if available in your version; otherwise comment out
        .flags = {
            .buff_dma = true,
            .swap_bytes = true,
        }};

    lv_disp_t *disp = lvgl_port_add_disp(&disp_cfg);
    (void)disp;
}

/**
 * Demonstrate cell placement and span
 */
void lv_example_grid_2(void)
{
    /*Column 1: fix width 120 px
     *Column 2: 1 unit from the remaining free space
     *Column 3: 2 unit from the remaining free space*/
    static int32_t col_dsc[] = {120, LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};

    /*Row 1: fix width 120 px
     *Row 2: 1 unit from the remaining free space
     *Row 3: 2 unit from the remaining free space*/
    static int32_t row_dsc[] = {120, LV_GRID_FR(1), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};

    /*Create a container with grid*/
    lv_obj_t *cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, EXAMPLE_LCD_V_RES, EXAMPLE_LCD_H_RES);
    lv_obj_center(cont);
    lv_obj_set_grid_dsc_array(cont, col_dsc, row_dsc);

    lv_obj_t *label;
    lv_obj_t *obj;
    uint8_t i;
    for (i = 0; i < 9; i++)
    {
        uint8_t col = i % 3;
        uint8_t row = i / 3;

        obj = lv_obj_create(cont);
        /*Stretch the cell horizontally and vertically too
         *Set span to 1 to make the cell 1 column/row sized*/
        lv_obj_set_grid_cell(obj, LV_GRID_ALIGN_STRETCH, col, 1,
                             LV_GRID_ALIGN_STRETCH, row, 1);

        label = lv_label_create(obj);
        lv_label_set_text_fmt(label, "%d,%d", col, row);
        lv_obj_center(label);
    }
}

/*void draw_acceleration_circles(lv_obj_t *parent, float acc_x, float acc_y, float acc_z)
{
    // Map acceleration values to circle sizes (this is just an example, adjust as needed)
    int size_x = (int)(acc_x * 20); // Scale factor for visualization
    int size_y = (int)(acc_y * 20);
    int size_z = (int)(acc_z * 20);

    // Create circles for each axis
    lv_obj_t *circle_x = lv_obj_create(parent);
    lv_obj_set_size(circle_x, size_x, size_x);
    lv_obj_set_style_bg_color(circle_x, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_radius(circle_x, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(circle_x, LV_ALIGN_CENTER, -50, 0);

    lv_obj_t *circle_y = lv_obj_create(parent);
    lv_obj_set_size(circle_y, size_y, size_y);
    lv_obj_set_style_bg_color(circle_y, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_set_style_radius(circle_y, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(circle_y, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *circle_z = lv_obj_create(parent);
    lv_obj_set_size(circle_z, size_z, size_z);
    lv_obj_set_style_bg_color(circle_z, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_radius(circle_z, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(circle_z, LV_ALIGN_CENTER, 50, 0);
}*/

void setup_ui()
{
    lvgl_port_lock(0);

    static lv_font_t mono_font_20;
    mono_font_20 = lv_font_montserrat_20;
    mono_font_20.get_glyph_dsc = fix_w_get_glyph_dsc_20;

    static lv_font_t mono_font_14;
    mono_font_14 = lv_font_montserrat_14;
    mono_font_14.get_glyph_dsc = fix_w_get_glyph_dsc_14;

    static lv_font_t mono_font_48;
    mono_font_48 = lv_font_montserrat_48;
    mono_font_48.get_glyph_dsc = fix_w_get_glyph_dsc_48;

    // speed label
    lbl_speed = lv_label_create(lv_screen_active()); // 20 and 28
    lv_obj_set_style_text_font(lbl_speed, &mono_font_48, 0);
    lv_obj_set_x(lbl_speed, 5);
    lv_obj_set_y(lbl_speed, 10);
    lv_label_set_text(lbl_speed, "0");

    // max speed
    lbl_max_speed = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_speed, &mono_font_20, 0);
    lv_obj_set_y(lbl_max_speed, 10);
    lv_obj_set_x(lbl_max_speed, 230);
    lv_label_set_text(lbl_max_speed, "0");

    lbl_0_100 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_0_100, &mono_font_20, 0);
    lv_obj_set_y(lbl_0_100, 30);
    lv_obj_set_x(lbl_0_100, 230);
    lv_label_set_text(lbl_0_100, "00.00s");

    lbl_80_100 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_80_100, &mono_font_20, 0);
    lv_obj_set_y(lbl_80_100, 50);
    lv_obj_set_x(lbl_80_100, 230);
    lv_label_set_text(lbl_80_100, "00.00s");

    lbl_80_120 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_80_120, &mono_font_20, 0);
    lv_obj_set_y(lbl_80_120, 70);
    lv_obj_set_x(lbl_80_120, 230);
    lv_label_set_text(lbl_80_120, "00.00s");

    // accelerations
    lbl_current_acc_x = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_current_acc_x, &mono_font_20, 0);
    lv_obj_set_x(lbl_current_acc_x, 5);
    lv_obj_set_y(lbl_current_acc_x, 100);
    lv_label_set_text(lbl_current_acc_x, "00.00");

    lbl_current_acc_y = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_current_acc_y, &mono_font_20, 0);
    lv_obj_set_x(lbl_current_acc_y, 5);
    lv_obj_set_y(lbl_current_acc_y, 120);
    lv_label_set_text(lbl_current_acc_y, "00.00");

    lbl_current_acc_z = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_current_acc_z, &mono_font_20, 0);
    lv_obj_set_x(lbl_current_acc_z, 5);
    lv_obj_set_y(lbl_current_acc_z, 140);
    lv_label_set_text(lbl_current_acc_z, "00.00");

    lbl_max_acc_x_p = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_x_p, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_x_p, 120);
    lv_obj_set_y(lbl_max_acc_x_p, 100);
    lv_label_set_text(lbl_max_acc_x_p, "00.00");

    lbl_max_acc_x_n = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_x_n, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_x_n, 230);
    lv_obj_set_y(lbl_max_acc_x_n, 100);
    lv_label_set_text(lbl_max_acc_x_n, "00.00");

    lbl_max_acc_y_p = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_y_p, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_y_p, 120);
    lv_obj_set_y(lbl_max_acc_y_p, 120);
    lv_label_set_text(lbl_max_acc_y_p, "00.00");

    lbl_max_acc_y_n = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_y_n, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_y_n, 230);
    lv_obj_set_y(lbl_max_acc_y_n, 120);
    lv_label_set_text(lbl_max_acc_y_n, "00.00");

    lbl_max_acc_z_p = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_z_p, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_z_p, 120);
    lv_obj_set_y(lbl_max_acc_z_p, 140);
    lv_label_set_text(lbl_max_acc_z_p, "00.00");

    lbl_max_acc_z_n = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_max_acc_z_n, &mono_font_20, 0);
    lv_obj_set_x(lbl_max_acc_z_n, 230);
    lv_obj_set_y(lbl_max_acc_z_n, 140);
    lv_label_set_text(lbl_max_acc_z_n, "00.00");

    lbl_gps_time = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_gps_time, &mono_font_20, 0);
    lv_obj_set_x(lbl_gps_time, 5);
    lv_obj_set_y(lbl_gps_time, 180);
    lv_label_set_text(lbl_gps_time, "00:00:00");

    lbl_gps_valid = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_gps_valid, &lv_font_montserrat_14, 0);
    lv_obj_set_x(lbl_gps_valid, 5);
    lv_obj_set_y(lbl_gps_valid, 210);
    lv_label_set_text(lbl_gps_valid, "GPS: No Fix");

    lv_obj_t *label2 = lv_label_create(lv_screen_active());
    // lv_obj_set_style_text_font(label2, &mono_font_20, 0);
    lv_obj_set_y(label2, 10);
    lv_obj_set_x(label2, 120);
    // lv_obj_set_style_text_font(label2, &mono_font, 0);
    lv_label_set_text(label2, "Max v");

    lv_obj_t *line1 = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points1[] = {{120, 29}, {230, 29}};
    lv_line_set_points(line1, line_points1, 2);
    lv_obj_set_style_line_color(line1, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_line_width(line1, 2, 0);
    lv_obj_set_style_line_rounded(line1, true, 0);

    lv_obj_t *label3 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label3, &mono_font_20, 0);
    lv_obj_set_x(label3, 120);
    lv_obj_set_y(label3, 30);
    lv_label_set_text(label3, " 0-100");

    lv_obj_t *line2 = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points2[] = {{120, 49}, {230, 49}};
    lv_line_set_points(line2, line_points2, 2);
    lv_obj_set_style_line_color(line2, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_line_width(line2, 2, 0);
    lv_obj_set_style_line_rounded(line2, true, 0);

    lv_obj_t *label4 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label4, &mono_font_20, 0);
    lv_obj_set_x(label4, 120);
    lv_obj_set_y(label4, 50);
    lv_label_set_text(label4, "80-100");

    lv_obj_t *line3 = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points3[] = {{120, 69}, {230, 69}};
    lv_line_set_points(line3, line_points3, 2);
    lv_obj_set_style_line_color(line3, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_line_width(line3, 2, 0);
    lv_obj_set_style_line_rounded(line3, true, 0);

    lv_obj_t *label5 = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label5, &mono_font_20, 0);
    lv_obj_set_x(label5, 120);
    lv_obj_set_y(label5, 70);
    lv_label_set_text(label5, "80-120");

    lv_obj_t *line4 = lv_line_create(lv_screen_active());
    static lv_point_precise_t line_points4[] = {{120, 89}, {230, 89}};
    lv_line_set_points(line4, line_points4, 2);
    lv_obj_set_style_line_color(line4, lv_color_hex(0x0000ff), 0);
    lv_obj_set_style_line_width(line4, 2, 0);
    lv_obj_set_style_line_rounded(line4, true, 0);

    lv_obj_t *lbl_version = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(lbl_version, &lv_font_montserrat_14, 0);
    lv_obj_set_x(lbl_version, 200);
    lv_obj_set_y(lbl_version, 210);
    // lv_obj_set_style_text_font(lbl_version, &mono_font, 0);
    lv_label_set_text(lbl_version, "CARACCEL v1.0");

    lvgl_port_unlock();
    /*

     tft.setTextSize(2);
      tft.setCursor(120, 10);
      tft.print("Max v");
      tft.drawLine(120, 25, 230, 25, ILI9341_BLUE);

      tft.setCursor(120, 30);
      tft.print("0-100");
      tft.setTextSize(1);
      tft.print("km/h");
      tft.setTextSize(2);
      tft.drawLine(120, 45, 230, 45, ILI9341_BLUE);

      tft.setCursor(120, 50);
      tft.print("80-100");
      tft.setTextSize(1);
      tft.print("km/h");
      tft.setTextSize(2);
      tft.drawLine(120, 65, 230, 65, ILI9341_BLUE);
      tft.setCursor(120, 70);
      tft.print("80-120");
      tft.setTextSize(1);
      tft.print("km/h");
      tft.setTextSize(2);
      tft.drawLine(120, 85, 230, 85, ILI9341_BLUE);
    */
}

void set_lbl_speed(const char *speed_str)
{

    lv_label_set_text(lbl_speed, speed_str);
}

void set_lbl_gps_time(const char *time_str)
{

    lv_label_set_text(lbl_gps_time, time_str);
}

void set_lbl_current_accelerations(float acc_x, float acc_y, float acc_z)
{
    char acc_str[32];
    if (acc_x >= 0)
    {
        snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_x);
    }
    else
    {
        snprintf(acc_str, sizeof(acc_str), "%.2f", acc_x);
    }
    lv_label_set_text(lbl_current_acc_x, acc_str);
    if (acc_y >= 0)
    {
        snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_y);
    }
    else
    {
        snprintf(acc_str, sizeof(acc_str), "%.2f", acc_y);
    }
    lv_label_set_text(lbl_current_acc_y, acc_str);
    if (acc_z >= 0)
    {
        snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_z);
    }
    else
    {
        snprintf(acc_str, sizeof(acc_str), "%.2f", acc_z);
    }
    lv_label_set_text(lbl_current_acc_z, acc_str);
}

void set_lbl_max_accelerations(float acc_x, float acc_y, float acc_z)
{
    char acc_str[32];
    snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_x);
    lv_label_set_text(lbl_max_acc_x_p, acc_str);

    snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_y);
    lv_label_set_text(lbl_max_acc_y_p, acc_str);

    snprintf(acc_str, sizeof(acc_str), "+%.2f", acc_z);
    lv_label_set_text(lbl_max_acc_z_p, acc_str);
}

void set_lbl_min_accelerations(float acc_x, float acc_y, float acc_z)
{
    char acc_str[32];
    snprintf(acc_str, sizeof(acc_str), "%.2f", acc_x);

    lv_label_set_text(lbl_max_acc_x_n, acc_str);
    snprintf(acc_str, sizeof(acc_str), "%.2f", acc_y);

    lv_label_set_text(lbl_max_acc_y_n, acc_str);
    snprintf(acc_str, sizeof(acc_str), "%.2f", acc_z);

    lv_label_set_text(lbl_max_acc_z_n, acc_str);
}

void set_lbl_gps_valid(bool gps_valid)
{
    static lv_style_t style;
    lv_style_init(&style);

    if (gps_valid)
    {
        // lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_GREEN));

        // v_obj_add_style(lbl_gps_valid, &style, 0);
        lv_label_set_text(lbl_gps_valid, "GPS: OK");
    }
    else
    {
        // lv_style_set_text_color(&style, lv_palette_main(LV_PALETTE_RED));

        // lv_obj_add_style(lbl_gps_valid, &style, 0);
        lv_label_set_text(lbl_gps_valid, "GPS: No Fix");
    }
}

void set_rotation(uint8_t m)
{
    uint8_t rotation = m % 4; // can't be higher than 3

    switch (rotation)
    {
    case 0:
        m = (MADCTL_MX | MADCTL_BGR);
        // _width = ILI9341_TFTWIDTH;
        // _height = ILI9341_TFTHEIGHT;
        break;
    case 1:
        m = (MADCTL_MV | MADCTL_BGR);
        // _width = ILI9341_TFTHEIGHT;
        // _height = ILI9341_TFTWIDTH;
        break;
    case 2:
        m = (MADCTL_MY | MADCTL_BGR);
        // _width = ILI9341_TFTWIDTH;
        // _height = ILI9341_TFTHEIGHT;
        break;
    case 3:
        m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
        // _width = ILI9341_TFTHEIGHT;
        // _height = ILI9341_TFTWIDTH;
        break;
    }

    // sendCommand(ILI9341_MADCTL, &m, 1);
}

void set_lbl_0_100(const char *time_str)
{
    lv_label_set_text(lbl_0_100, time_str);
}

void set_lbl_80_100(const char *time_str)
{
    lv_label_set_text(lbl_80_100, time_str);
}

void set_lbl_80_120(const char *time_str)
{
    lv_label_set_text(lbl_80_120, time_str);
}

void set_lbl_max_speed(const char *speed_str)
{
    lv_label_set_text(lbl_max_speed, speed_str);
}

// Draws vertical bars where positive acceleration is up and negative acceleration is down
// Are drawn in red for X, green for Y and blue for Z
// Position is fixed and length is proportional to acceleration value
// Maximum length corresponds to a 4G
void draw_acceleration_bars(float acc_x, float acc_y, float acc_z, uint16_t pos_x, uint16_t pos_y)
{
    // Map acceleration values to bar lengths (this is just an example, adjust as needed)
    int max_length = 50; // Maximum length for 4G
    int length_x = (int)(acc_x / 4.0 * max_length);
    int length_y = (int)(acc_y / 4.0 * max_length);
    int length_z = (int)(acc_z / 4.0 * max_length);
}