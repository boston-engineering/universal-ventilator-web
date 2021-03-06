#ifndef UVENT_UTIL_H
#define UVENT_UTIL_H

#include <cstdint>
#include "lvgl/lvgl.h"
#include <lvgl/src/hal/lv_hal_tick.h>

#define EPSILON 0.0000001

/**
 * @param ptr The pointer to the field/var keeping the time
 * @param n The amount of millis required to elapse
 * @return True if n millis have passed, else false.
 */
bool has_time_elapsed(uint32_t* ptr, uint32_t n);

// Returns the current time in seconds
inline float now_s() { return lv_tick_get() * 1e-3; }

bool is_whole(double x, double epsilon = EPSILON);

#endif //UVENT_UTIL_H
