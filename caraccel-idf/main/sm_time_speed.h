#pragma once
#include "esp_timer.h"

void sm_time_speed_init();

bool sm_time_speed_update(uint64_t time, double speed_kph);

void sm_time_speed_get_deltas(double *delta_0_80_s, double *delta_80_100_s, double *delta_80_120_s);