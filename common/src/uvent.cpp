#include "uvent.h"


MainScreen main_screen;
StartupScreen startup_screen;
// Timers
lv_timer_t* update_readout_timer = nullptr;

static void on_startup_confirm_button(lv_event_t* evt)
{
    LV_UNUSED(evt);
    // Destroy the startup screen
    startup_screen.cleanup();
    // Need to be able to create new windows later
    active_floating_window = nullptr;
    // Tell LVGL this is the currently loaded screen
    main_screen.select_screen();
    // Init containers, styles, defaults...
    init_main_display();
    // Creates all the components that go on the main screen in order for it to function.
    main_screen.setup();
    // Arm the speaker so it talks to LVGL on mute/unmute
    control_setup_alarm_cb();

    update_readout_timer = lv_timer_create(loop_test_readout, SENSOR_POLL_INTERVAL, &main_screen);
}

static void setup_screens()
{
    // Create the screen objects
    startup_screen.init();
    main_screen.init();

    startup_screen.select_screen();
    startup_screen.on_complete = on_startup_confirm_button;
    startup_screen.setup();
}

// TODO clean up setup & main loop

void uvent_setup()
{
    init_adjustable_values();
    setup_screens();
}