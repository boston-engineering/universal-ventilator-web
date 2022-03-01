#ifndef UVENT_UTIL_H
#define UVENT_UTIL_H

#include <cstdint>
#include <vector>
#include "lvgl/lvgl.h"
#include <lvgl/src/hal/lv_hal_tick.h>

#define EPSILON 0.0000001

struct FakeData {
    double time_step;
    double flow;
    double delivered_vt;
    double lung_vol;
    double lung_pressure;
    double pressure_premask;
};

/**
 * @param ptr The pointer to the field/var keeping the time
 * @param n The amount of millis required to elapse
 * @return True if n millis have passed, else false.
 */
bool has_time_elapsed(uint32_t* ptr, uint32_t n);

// Returns the current time in seconds
inline float now_s() { return (float) (static_cast<float>(lv_tick_get()) * 1e-3); }

bool is_whole(double x, double epsilon = EPSILON);

std::vector<FakeData> load_fake_data();

#endif//UVENT_UTIL_H
