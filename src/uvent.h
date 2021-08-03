#include "lvgl/lvgl.h"
#include "display/screens/screen.h"
#include "utilities/util.h"
#include "display/layouts/layouts.h"
#include "uvent_conf.h"
#include "controls/control.h"
#include "sensors/pressure_sensor.h"
#include "sensors/test_pressure_sensors.h"
#include "display/main_display.h"

extern MainScreen main_screen;
extern StartupScreen startup_screen;
// Timers
extern lv_timer_t* update_readout_timer;

void uvent_setup();