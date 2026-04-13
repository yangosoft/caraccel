#pragma once

#include "esp_lcd_ili9341.h"
// #include "esp_lcd_ili9341_init_cmds_2.h"
//  #include "esp_lcd_ili9341_init_cmds_2.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

void init_ili9341(esp_lcd_panel_handle_t panel_handle);

void setup_ui();

void set_lbl_gps_time(const char *time_str);

void set_lbl_current_accelerations(float acc_x, float acc_y, float acc_z);

void set_lbl_max_accelerations(float acc_x, float acc_y, float acc_z);
void set_lbl_min_accelerations(float acc_x, float acc_y, float acc_z);

void set_lbl_speed(const char *speed_str);
void set_lbl_max_speed(const char *speed_str);

void set_lbl_gps_valid(bool gps_valid);

void set_lbl_0_100(const char *timing_str);
void set_lbl_80_100(const char *timing_str);
void set_lbl_80_120(const char *timing_str);