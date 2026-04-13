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

void sm_time_speed_init()
{
    state = kInit;
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