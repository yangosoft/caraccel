#pragma once
#include "esp_timer.h"

#define MAX_NUMBER_OF_SAMPLES 128

struct time_speed_data_t
{
    uint16_t time_0_80_ms;
    uint16_t time_80_100_ms;
    uint16_t time_80_120_ms;
}; // size with padding is 6 bytes,
// so 128 samples will be 768 bytes

void sm_time_speed_init();

void sm_time_speed_start();

bool sm_time_speed_update(uint64_t time, double speed_kph);

void sm_time_speed_get_deltas(double *delta_0_80_s, double *delta_80_100_s, double *delta_80_120_s);

void sm_time_speed_get_history(const struct time_speed_data_t **out_history, uint16_t *out_num_samples);