#include <random>
#include "control.h"
#include "waveform.h"
#include "../display/screens/screen.h"
#include "../utilities/util.h"
#include "../display/layouts/layouts.h"
#include "../eeprom/storage.h"
#include "../sensors/pressure_sensor.h"
#include "../uvent_conf.h"
#include "../actuators/actuator.h"
#include "../alarm/alarm.h"

// Instance to control the paddle
Actuator actuator;

// Storage instance
Storage storage;

// Gauge Pressure Sensor instance
PressureSensor gauge_sensor = {PRESSURE_GAUGE_PIN};

// Differential Pressure Sensor instance
PressureSensor diff_sensor = {PRESSURE_DIFF_PIN};
// Waveform instance
Waveform waveform;

uint32_t cycle_count = 0;

// Alarms
AlarmManager alarm_manager{SPEAKER_PIN, &cycle_count};

/* State machine instance. Takes in a pointer to actuator
 * as there are actuator commands within the state machine.
 */
// Machine machine(States::ST_STARTUP, &actuator, &waveform, &alarm);
Machine machine(States::ST_STARTUP, &actuator, &waveform, &alarm_manager, &cycle_count);

// Bool to keep track of the alert box
static bool alert_box_already_visible = false;

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(0, 100);

void loop_simulate_readouts(lv_timer_t* timer)
{

    static bool timer_delay_complete = false;

    // Internal timers, components might have different refresh times
    static uint32_t last_readout_refresh = 0;

    // Don't poll the sensors before we're sure everything's had a chance to init
    if (!timer_delay_complete && (lv_tick_get() >= SENSOR_POLL_STARTUP_DELAY)) {
        timer_delay_complete = true;

        // Set some dummy alarms
        cycle_count++;

        alarm_manager.badPlateau(true);
        alarm_manager.lowPressure(true);
        alarm_manager.noTidalPres(true);
        alarm_manager.update();
    }
    if (!timer_delay_complete) {
        LV_LOG_TRACE("Timer is not ready yet, returning (%d)", millis());
        return;
    }

    // If verbose data polling is off, polling / updates won't be done if the state machine is off
    // If in debug screen mode, it won't be done if we're in startup state either
#if VERBOSE_DATA_POLLING == 0
    States cur_state = control_get_state();
    if (cur_state == States::ST_OFF || (!ENABLE_CONTROL && cur_state == States::ST_STARTUP)) {
        return;
    }
#endif

    // Main screen, passed through via user data in main.cpp
    auto* screen = static_cast<MainScreen*>(timer->user_data);

    // Check for errors
    handle_alerts();

    // Poll gauge sensor, add point to graph and update readout obj.
    // Will not refresh until explicitly told
    static double cur_pressure = 0;
    screen->get_chart(CHART_IDX_PRESSURE)->add_data_point(cur_pressure);
    set_readout(AdjValueType::CUR_PRESSURE, cur_pressure);
    set_readout(AdjValueType::PRES, cur_pressure);
    screen->get_chart(CHART_IDX_FLOW)->add_data_point(cur_pressure);
    set_readout(AdjValueType::FLOW, cur_pressure);
    cur_pressure += 1;
    double rand = (distribution(generator) / 100.0);
    cur_pressure += rand;
    if (cur_pressure > 40) {
        cur_pressure -= 42;
    }

    // Poll vT sensor, add point to graph and update readout obj.
    // Will not refresh until explicitly told
    static int16_t test2 = 0;
    double cur_tidal_volume = test2;
    set_readout(AdjValueType::TIDAL_VOLUME, cur_tidal_volume);
    test2 += 50;
    if (test2 > 1000) {
        test2 -= 1002;
    }

    set_readout(RESPIRATION_RATE, 30);

    set_readout(PEEP, 30);
    set_readout(PIP, 30);
    set_readout(IE_RATIO_LEFT, 1.2);
    set_readout(IE_RATIO_RIGHT, 2.0);

    // Check to see if it's time to refresh the readout boxes
    if (has_time_elapsed(&last_readout_refresh, READOUT_REFRESH_INTERVAL)) {
        // Refresh all of the readout labels
        for (auto& value : adjustable_values) {
            if (!value.is_dirty()) {
                continue;
            }
            value.refresh_readout();
            value.clear_dirty();
        }
    }

    screen->try_refresh_charts();
}

void loop_update_readouts(lv_timer_t* timer)
{
    static bool timer_delay_complete = false;

    // Internal timers, components might have different refresh times
    static uint32_t last_readout_refresh = 0;

    // Don't poll the sensors before we're sure everything's had a chance to init
    if (!timer_delay_complete && (lv_tick_get() >= SENSOR_POLL_STARTUP_DELAY)) {
        timer_delay_complete = true;
    }
    if (!timer_delay_complete) {
        LV_LOG_TRACE("Timer is not ready yet, returning (%d)", millis());
        return;
    }

    // If verbose data polling is off, polling / updates won't be done if the state machine is off
    // If in debug screen mode, it won't be done if we're in startup state either
#if VERBOSE_DATA_POLLING == 0
    States cur_state = control_get_state();
    if (cur_state == States::ST_OFF || (!ENABLE_CONTROL && cur_state == States::ST_STARTUP)) {
        return;
    }
#endif

    // Main screen, passed through via user data in main.cpp
    auto* screen = static_cast<MainScreen*>(timer->user_data);

    // Check for errors
    handle_alerts();

    // Poll gauge sensor, add point to graph and update readout obj.
    // Will not refresh until explicitly told
    double cur_pressure = control_get_gauge_pressure();
    screen->get_chart(CHART_IDX_PRESSURE)->add_data_point(cur_pressure);
    set_readout(AdjValueType::CUR_PRESSURE, cur_pressure);

    // Poll vT sensor, update readout obj.
    // Will not refresh until explicitly told
    double cur_tidal_volume = control_get_degrees_to_volume_ml();
    set_readout(AdjValueType::TIDAL_VOLUME, cur_tidal_volume);

    // TODO add more sensors HERE

    // Check to see if it's time to refresh the readout boxes
    if (has_time_elapsed(&last_readout_refresh, READOUT_REFRESH_INTERVAL)) {
        // Refresh all of the readout labels
        for (auto& value : adjustable_values) {
            if (!value.is_dirty()) {
                continue;
            }
            value.refresh_readout();
            value.clear_dirty();
        }
    }

    screen->try_refresh_charts();
}

void handle_alerts()
{
    static uint16_t last_alarm_count = 0;
    uint16_t alarm_count = control_get_alarm_count();
    if (alarm_count <= 0 && alert_box_already_visible) {
        alert_box_already_visible = false;
        last_alarm_count = 0;
        set_alert_count_visual(0);
        set_alert_text("");
        set_alert_box_visible(false);
        return;
    }

    if (alarm_count > 0 && !alert_box_already_visible) {
        alert_box_already_visible = true;
        set_alert_box_visible(true);
    }

    if (last_alarm_count != alarm_count) {
        last_alarm_count = alarm_count;

        Alarm* alarm_arr = control_get_alarm_list();
        std::string alarm_strings[NUM_ALARMS];
        uint16_t buf_size = 0;
        uint16_t alarm_list_idx = 0;
        for (uint16_t i = 0; i < NUM_ALARMS; i++) {
            Alarm* a = &alarm_arr[i];
            if (!a->isON()) {
                continue;
            }
            buf_size += a->text().length();
            alarm_strings[alarm_list_idx++] = a->text();
        }

        set_alert_count_visual(alarm_count);
        set_alert_text(alarm_strings, alarm_count, buf_size);
    }
}

void control_update_waveform_param(AdjValueType type, float new_value)
{
    if (type >= AdjValueType::ADJ_VALUE_COUNT) {
        return;
    }
    waveform_params* wave_params = control_get_waveform_params();
    switch (type) {
        case TIDAL_VOLUME:
            wave_params->volume_ml = new_value;
            LV_LOG_INFO("Tidal Volume is now %.1f", new_value);
            break;
        case RESPIRATION_RATE:
            wave_params->bpm = (uint16_t) new_value;
            LV_LOG_INFO("Respiration Rate is now %.1f", new_value);
            break;
        case PIP:
            wave_params->pip = (uint16_t) new_value;
            LV_LOG_INFO("PIP is now %.1f", new_value);
            break;
        case PEEP:
            wave_params->peep = (uint16_t) new_value;
            LV_LOG_INFO("PEEP is now %.1f", new_value);
            break;
        case PLATEAU_TIME:
            // TODO
            LV_LOG_INFO("TO ADD: PLATEAU TIME");
            break;
        case IE_RATIO_LEFT:
            wave_params->ie_i = new_value;
            LV_LOG_INFO("IE Inspiration is now %.1f", new_value);
            break;
        case IE_RATIO_RIGHT:
            wave_params->ie_e = new_value;
            LV_LOG_INFO("IE Expiration is now %.1f", new_value);
            break;
        default:
            break;
    }
    control_calculate_waveform();
}

/**
 * Loads a target value from EEPROM at startup and sets it to its interface class.
 * This function will recalculate the waveform itself, no other action is needed.
 * @param value The value we're loading.
 * @param settings Pre-Loaded settings object to retrieve from.
 */
static void load_stored_target(AdjustableValue* value, uvent_settings& settings)
{
    double val = 0;
    switch (value->value_type) {
        case TIDAL_VOLUME:
            val = settings.tidal_volume;
            break;
        case RESPIRATION_RATE:
            val = settings.respiration_rate;
            break;
        case PEEP:
            val = settings.peep_limit;
            break;
        case PIP:
            val = settings.pip_limit;
            break;
        case PLATEAU_TIME:
            val = settings.plateau_time;
            break;
        case IE_RATIO_LEFT:
            val = settings.ie_ratio_left;
            break;
        case IE_RATIO_RIGHT:
            val = settings.ie_ratio_right;
            break;
        default:
            break;
    }
    value->set_value_target(val);
    control_update_waveform_param(value->value_type, (float) val);
}

void init_adjustable_values()
{
    uvent_settings settings{};
    storage.get_settings(settings);

    for (uint8_t i = 0; i < AdjValueType::ADJ_VALUE_COUNT; i++) {
        AdjustableValue* value_class = &adjustable_values[i];
        value_class->init(static_cast<AdjValueType>(i));
        load_stored_target(value_class, settings);
        value_class->set_value_measured(READOUT_VALUE_DEFAULT);
    }
    adjustable_values[AdjValueType::IE_RATIO_LEFT].set_selected(false);
}

double get_control_target(AdjValueType type)
{
    if (type >= AdjValueType::ADJ_VALUE_COUNT) {

        return adjustable_values[type].get_settings().default_value;
    }

    return *adjustable_values[type].get_value_target();
}

void set_readout(AdjValueType type, double val)
{
    if (type >= AdjValueType::ADJ_VALUE_COUNT) {
        return;
    }
    adjustable_values[type].set_value_measured(val);
}

double get_readout(AdjValueType type)
{
    if (type >= AdjValueType::ADJ_VALUE_COUNT) {
        return -1;
    }
    return *adjustable_values[type].get_value_measured();
}

void control_handler_cb()
{
    static bool ledOn = false;

    // Toggle the LED.
    ledOn = !ledOn;

    // Run the state machine
    machine.run();
}

/* Interrupt callback to service the actuator
 * The actuator, in this case a stepper is serviced periodically
 * to "step" the motor.
 */
void actuator_handler_cb()
{
    actuator.run();
}

/* Initilaize sub-systems
 */
void control_init()
{
#if ENABLE_CONTROL

    //  Storage init
    storage.init();

    // Initialize the actuator
    actuator.init();

    /* On startup, the angle feedback sensor needs to be programmed with a calibrated
     * zero. Usually this is an OTP value burned into the angle register, but it is risky to
     * do that, as the magnet may fall or go out-of alignment during transport and the zero cannot
     * be programmed again(because OTP).
     * To avoid the risk, the EEPROM stores the zero value, which is burned in during manufacture.
     * This value can be changed by manually "re-homing" the actuator at the user's discretion.
     *
     * The below few lines, load the zero value from the EEPROM into the angle sensor register
     * at statup. Control can then use the value from the angle sensor to home the actuator.
     */
    uvent_settings settings;
    storage.get_settings(settings);
    actuator.set_zero_position(settings.actuator_home_offset_adc_counts);

    // Initialize the Gauge Pressure Sensor
    gauge_sensor.init(MAX_GAUGE_PRESSURE, MIN_GAUGE_PRESSURE, RESISTANCE_1, RESISTANCE_2, settings.gpressure_offset_adc_counts);

    // Initialize the Differential Pressure Sensor
    if (settings.diff_pressure_type == PRESSURE_SENSOR_TYPE_0) {
        diff_sensor.init(MAX_DIFF_PRESSURE_TYPE_0, MIN_DIFF_PRESSURE_TYPE_0, RESISTANCE_1, RESISTANCE_2,
                         settings.dpressure_offset_adc_counts);
    }
    else if (settings.diff_pressure_type == PRESSURE_SENSOR_TYPE_1) {
        diff_sensor.init(MAX_DIFF_PRESSURE_TYPE_1, MIN_DIFF_PRESSURE_TYPE_1, RESISTANCE_1, RESISTANCE_2,
                         settings.dpressure_offset_adc_counts);
    }

    // Initialize the state machine
    machine.setup();

    /* Setup a timer and a function handler to run
     * the state machine.
     */
//    Timer0.attachInterrupt(control_handler);
//    Timer0.start(CONTROL_HANDLER_PERIOD_US);

    /* Setup a timer and a function handler to run
     * the actuation.
     */
//    Timer1.attachInterrupt(actuator_handler);
//    Timer1.start(ACTUATOR_HANDLER_PERIOD_US);

    // Check EEPROM CRC. Load defaults if CRC fails.
    if (!storage.is_crc_ok()) {
        // No settings found, or settings corrupted.
        storage.load_defaults();
    }
#endif
}

/* Called by loop() from main, this function services control
 * parametes and functions.
 * Since this is called by the loop(), the call interval
 * in not periodic.
 */
void control_service()
{
}

/* Get the current angular position of the actuator
 */
double control_get_actuator_position()
{
    return actuator.get_position();
}

/* Get the current raw register value of the angle sensor.
 * This is a debug stub used to get the offset to set home.
 */
int8_t control_get_actuator_position_raw(double& angle)
{
    return actuator.get_position_raw(angle);
}

/**
 * DO NOT USE UNLESS ABSOLUTELY NECESSARY
 */
void control_eeprom_write_default()
{

}

/* Set the current angular position of the actuator as home
 */
void control_zero_actuator_position()
{
    uvent_settings settings;
    storage.get_settings(settings);

    // Zero the angle sensor and write the value to the EEPROM
    settings.actuator_home_offset_adc_counts = actuator.set_current_position_as_zero();

    // Store this value to the eeprom.
}

void control_write_ventilator_params()
{
    uvent_settings settings;
    storage.get_settings(settings);

    settings.tidal_volume = (uint16_t) get_control_target(TIDAL_VOLUME);
    settings.respiration_rate = (uint8_t) get_control_target(RESPIRATION_RATE);
    settings.peep_limit = (uint8_t) get_control_target(PEEP);
    settings.pip_limit = (uint8_t) get_control_target(PIP);
    settings.plateau_time = (uint16_t) get_control_target(PLATEAU_TIME);
    settings.ie_ratio_left = get_control_target(IE_RATIO_LEFT);
    settings.ie_ratio_right = get_control_target(IE_RATIO_RIGHT);

}

void control_get_serial(char* serial_buffer)
{
    uvent_settings settings{};
    storage.get_settings(settings);

    memcpy(serial_buffer, settings.serial, 12);
}

/* Request to switch the state of the state machine.
 */
void control_change_state(States new_state)
{
    machine.change_state(new_state);
}

void control_actuator_manual_move(Tick_Type tt, double angle, double speed)
{
    actuator.set_position_relative(tt, angle);
    actuator.set_speed(tt, speed);
}

States control_get_state()
{
    return machine.get_current_state();
}

const char* control_get_state_string()
{
    return machine.get_current_state_string();
}

const char** control_get_state_list(uint8_t* size)
{
    return machine.get_state_list(size);
}

void control_display_storage()
{
    storage.display_storage();
}

double control_get_degrees_to_volume(C_Stat compliance)
{
    return actuator.degrees_to_volume(compliance);
}

double control_get_degrees_to_volume_ml(C_Stat compliance)
{
    return actuator.degrees_to_volume(compliance) * 1000;
}

double control_calc_volume_to_degrees(C_Stat compliance, double volume)
{
    return actuator.volume_to_degrees(compliance, volume);
}

void control_actuator_set_enable(bool en)
{
    actuator.set_enable(en);
}

waveform_params* control_get_waveform_params(void)
{
    return (waveform.get_params());
}

void control_calculate_waveform()
{
    waveform.calculate_waveform();
}

void control_waveform_display_details()
{
    waveform.display_details();
}

double control_get_gauge_pressure()
{
    return gauge_sensor.get_pressure(units_pressure::cmH20);
}

double control_get_diff_pressure()
{
    return diff_sensor.get_pressure(units_pressure::cmH20);
}

void control_setup_alarm_cb()
{
    alarm_manager.set_snooze_cb([]() {
        lv_obj_t* mute_button = get_mute_button();
        if (!mute_button) {
            return;
        }

        lv_obj_clear_state(mute_button, LV_STATE_CHECKED);
    });
}

void control_alarm_snooze()
{
    alarm_manager.snooze();
}

void control_toggle_alarm_snooze()
{
    alarm_manager.toggle_snooze();
}

int16_t control_get_alarm_count()
{
    return alarm_manager.numON();
}

std::string control_get_alarm_text()
{
    return (alarm_manager.getText());
}

void control_set_alarm_all_off()
{
    alarm_manager.allOff();
}

Alarm* control_get_alarm_list()
{
    return alarm_manager.getAlarmList();
}

void control_alarm_test()
{
    alarm_manager.overCurrent(true);
}