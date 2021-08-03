#include <cmath>
#include "util.h"

bool has_time_elapsed(uint32_t* timer_ptr, uint32_t n)
{
    uint32_t ms = lv_tick_get();
    bool result = ms - *timer_ptr >= n;

    if (ms < *timer_ptr || result) {
        *timer_ptr = ms;
        return true;
    }

    return false;
}

bool is_whole(double x, double epsilon)
{
    return abs(x - floor(x)) < epsilon;
}