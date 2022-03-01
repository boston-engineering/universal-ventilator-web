
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */

#include <cstdlib>
#include <unistd.h>

#define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

#include <SDL2/SDL.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/mouse.h"
#include "lv_drivers/indev/keyboard.h"
#include "lv_drivers/indev/mousewheel.h"
#include "../common/src/uvent.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);

static int tick_thread(void *data);

static int control_us_timer(void *data);

static int actuator_us_timer(void *data);

std::vector<FakeData> load_fake_data();

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int main(int argc, char **argv) {
    (void) argc; /*Unused*/
    (void) argv; /*Unused*/

    /*Initialize LVGL*/
    lv_init();

    /*Initialize the HAL (display, input devices, tick) for LVGL*/
    hal_init();

    /* Init the audio system to generate alarm tones */
    SDL_Init(SDL_INIT_AUDIO);

    load_fake_data();

    uvent_setup();

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (true) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_timer_handler();
        usleep(5 * 1000);
    }
#pragma clang diagnostic pop

    return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static void hal_init() {
    /* Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();
    /* Tick init.
     * You have to call 'lv_tick_inc()' in periodically to inform LittelvGL about
     * how much time were elapsed Create an SDL thread to do this*/
    SDL_CreateThread(tick_thread, "tick", nullptr);
    SDL_CreateThread(control_us_timer, "control_timer", nullptr);
    SDL_CreateThread(actuator_us_timer, "actuator_timer", nullptr);

    /*Create a display buffer*/
    static lv_disp_draw_buf_t disp_buf1;
    static lv_color_t buf1_1[MONITOR_HOR_RES * 100];
    static lv_color_t buf1_2[MONITOR_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf1, buf1_1, buf1_2, MONITOR_HOR_RES * 100);

    /*Create a display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv); /*Basic initialization*/
    disp_drv.draw_buf = &disp_buf1;
    disp_drv.flush_cb = monitor_flush;
    disp_drv.hor_res = MONITOR_HOR_RES;
    disp_drv.ver_res = MONITOR_VER_RES;
    disp_drv.antialiasing = 1;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    lv_theme_t *th = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                                           LV_THEME_DEFAULT_DARK, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, th);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    /* Add the mouse as input device
     * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = mouse_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);

    keyboard_init();
    static lv_indev_drv_t indev_drv_2;
    lv_indev_drv_init(&indev_drv_2); /*Basic initialization*/
    indev_drv_2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv_2.read_cb = keyboard_read;
    lv_indev_t *kb_indev = lv_indev_drv_register(&indev_drv_2);
    lv_indev_set_group(kb_indev, g);
    mousewheel_init();
    static lv_indev_drv_t indev_drv_3;
    lv_indev_drv_init(&indev_drv_3); /*Basic initialization*/
    indev_drv_3.type = LV_INDEV_TYPE_ENCODER;
    indev_drv_3.read_cb = mousewheel_read;

    lv_indev_t *enc_indev = lv_indev_drv_register(&indev_drv_3);
    lv_indev_set_group(enc_indev, g);

    /*Set a cursor for the mouse*/
    LV_IMG_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
    lv_obj_t *cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/
}

std::vector<FakeData> load_fake_data() {
    std::vector<FakeData> data;
    std::ifstream file("../../common/data/ScopeData.csv");

    if(!file) {
        return data;
    }

    std::string line;
    bool first = true;
    while(std::getline(file, line)) {
        // Exclude the header of the csv
        if(first) {
            first = false;
            continue;
        }

        FakeData data_point{};
        uint offset = 0;
        std::string token;
        std::istringstream token_stream(line);
        while(std::getline(token_stream, token, ',')) {
            try{
                double val = std::stod(token);
                // Since the struct is just POD, shove in via an offset
                *((double*) (((char *) &data_point) + (sizeof(double) * offset))) = val;
            }catch(const std::invalid_argument&){
                break;
            }catch(const std::out_of_range&){
                break;
            }
            offset++;
        }

        // Skip if we don't have enough data in the line
        if(offset < 6) {
            continue;
        }

        data.push_back(data_point);
    }

    file.close();
    std::cout << "File Closed" << std::endl;
    return data;
}

/**
 * A task to measure the elapsed time for LVGL
 * @param data unused
 * @return never return
 */
#pragma clang diagnostics push
#pragma ide diagnostic ignored "EndlessLoop"

static int tick_thread(void *data) {
    (void) data;

    static timespec nano_timespec = {
            0,
            5000000,
            };

    while (true) {
        nanosleep(&nano_timespec, nullptr);
        lv_tick_inc(5); /*Tell LittelvGL that 5 milliseconds were elapsed*/
    }

    return 0;
}

static int control_us_timer(void *data) {
    (void) data;

    static timespec nano_timespec = {
            0,
            CONTROL_HANDLER_PERIOD_US * 1000,
    };

    while (true) {
        nanosleep(&nano_timespec, nullptr);
        control_handler();
    }

    return 0;
}

static int actuator_us_timer(void *data) {
    (void) data;

    static timespec nano_timespec = {
            0,
            ACTUATOR_HANDLER_PERIOD_US * 1000,
    };

    while (true) {
        nanosleep(&nano_timespec, nullptr);
        actuator_handler();
    }

    return 0;
}

#pragma clang diagnostics pop
