#include "sm_time_speed.h"

enum MeassureState
{
    kInit,
    kV80,
    kV100,
    kV120,
    kSave
};

static enum MeassureState state = kInit;
static uint64_t time_0 = 0;
static uint64_t time_80 = 0;
static uint64_t time_100 = 0;
static uint64_t time_120 = 0;
static double delta_0_100_s = 0;
static double delta_0_80_s = 0;
static double delta_80_100_s = 0;
static double delta_80_120_s = 0;

static struct time_speed_data_t history_time_speed_data[MAX_NUMBER_OF_SAMPLES];

static uint16_t current_sample_idx = 0;

void sm_time_speed_init()
{
    state = kInit;
    // clear historial data
    for (int i = 0; i < MAX_NUMBER_OF_SAMPLES; i++)
    {
        history_time_speed_data[i].time_0_80_ms = 0;
        history_time_speed_data[i].time_80_100_ms = 0;
        history_time_speed_data[i].time_80_120_ms = 0;
    }
    current_sample_idx = 0;
    sm_time_speed_start();
}

void sm_time_speed_start()
{
    state = kInit;
    time_0 = 0;
    time_80 = 0;
    time_100 = 0;
    time_120 = 0;
    delta_0_100_s = 0;
    delta_0_80_s = 0;
    delta_80_100_s = 0;
    delta_80_120_s = 0;

    // lets fill some ramdon data in history to test BLE transmission
    for (int i = 0; i < MAX_NUMBER_OF_SAMPLES; i++)
    {
        history_time_speed_data[i].time_0_80_ms = 1000 + i;  // 1s to 80km/h
        history_time_speed_data[i].time_80_100_ms = 500 + i; // 0.5s from 80 to 100km/h
        history_time_speed_data[i].time_80_120_ms = 800 + i; // 0.8s from 80 to 120km/h
    }
    current_sample_idx = MAX_NUMBER_OF_SAMPLES; // pretend we have a full history for testing
}

bool sm_time_speed_update(uint64_t time, double speed_kph)
{
    switch (state)
    {
    case kInit:
        if (speed_kph >= 7 && speed_kph < 10)
        {
            time_0 = time;
            state = kInit;
        }
        if (speed_kph >= 80)
        {
            time_80 = time;
            state = kV80;

            uint64_t delta_0_80 = time_80 - time_0;
            delta_0_80_s = delta_0_80 / 1e6;
        }
        break;
    case kV80:
        if (speed_kph >= 100)
        {
            time_100 = time;
            state = kV100;
            uint64_t delta_80_100 = time_100 - time_80;
            delta_80_100_s = delta_80_100 / 1e6;

            return true;
        }
        break;
    case kV100:
        if (speed_kph >= 120)
        {
            time_120 = time;
            state = kV120;
            uint64_t delta_80_120 = time_120 - time_80;
            delta_80_120_s = delta_80_120 / 1e6;
            uint64_t delta_0_100 = time_100 - time_0;
            delta_0_100_s = delta_0_100 / 1e6;
            return true;
        }
        break;
    case kV120:
        if (speed_kph < 120)
        {
            state = kSave;

            uint64_t delta_80_100 = time_100 - time_80;
            uint64_t delta_80_120 = time_120 - time_80;

            // Convert to seconds

            delta_80_100_s = delta_80_100 / 1e6;
            delta_80_120_s = delta_80_120 / 1e6;

            if (current_sample_idx < MAX_NUMBER_OF_SAMPLES)
            {
                // stores in ms to save space, as we don't need more precision and it allows us to store more samples in memory
                history_time_speed_data[current_sample_idx].time_0_80_ms = (uint16_t)(delta_0_80_s * 1000);
                history_time_speed_data[current_sample_idx].time_80_100_ms = (uint16_t)(delta_80_100_s * 1000);
                history_time_speed_data[current_sample_idx].time_80_120_ms = (uint16_t)(delta_80_120_s * 1000);
                current_sample_idx++;
            }

            // Log results
            return true;
        }
        break;
    case kSave:

        if (speed_kph < 10)
        {
            state = kInit;
        }

        break;
    }

    return false;
}

void sm_time_speed_get_deltas(double *out_delta_0_100_s, double *out_delta_80_100_s, double *out_delta_80_120_s)
{
    if (out_delta_0_100_s)
    {
        *out_delta_0_100_s = delta_0_100_s;
    }
    if (out_delta_80_100_s)
    {
        *out_delta_80_100_s = delta_80_100_s;
    }
    if (out_delta_80_120_s)
    {
        *out_delta_80_120_s = delta_80_120_s;
    }
}

void sm_time_speed_get_history(const struct time_speed_data_t **out_history, uint16_t *out_num_samples)
{
    if (out_history && out_num_samples)
    {
        *out_history = history_time_speed_data;
        *out_num_samples = current_sample_idx;
    }
}