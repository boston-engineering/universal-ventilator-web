
#include "machine.h"
#include "../actuators/actuator.h"
#include "../utilities/util.h"

// Stringify states
const char* state_string[] =
        {
                stringify(ST_STARTUP),
                stringify(ST_INSPR),
                stringify(ST_INSPR_HOLD),
                stringify(ST_EXPR),
                stringify(ST_PEEP_PAUSE),
                stringify(ST_EXPR_HOLD),
                stringify(ST_ACTUATOR_HOME),
                stringify(ST_ACTUATOR_JOG),
                stringify(ST_FAULT),
                stringify(ST_DEBUG),
                stringify(ST_OFF)};

// Machine::Machine(States st, Actuator* act, Waveform* wave, AlarmManager* al)
Machine::Machine(States st, Actuator* act, Waveform* wave, AlarmManager* al, uint32_t* cc)
{
    p_actuator = act;
    state = st;

    p_waveform = wave;
    p_waveparams = wave->get_params();
    // Calculate the waveform parameters
    p_waveform->calculate_waveform();

    p_alarm_manager = al;
    cycle_count = cc;
}

// Set the current state in the state machine
void Machine::set_state(States new_state)
{
    state_first_entry = true;
    state = new_state;

    // Reset the timer before each state starts.
    machine_timer = 0;
}

// State functions
void Machine::state_startup()
{
    if (state_first_entry) {
        state_first_entry = false;
    }

    // For testing. Each tick is CONTROL_HANDLER_PERIOD_US
    if (machine_timer > start_home_in_ticks) {
        set_state(States::ST_OFF);
    }
}

void Machine::state_inspiration()
{
    if (state_first_entry) {

        // Check if paddle is at home.
        if (!(p_actuator->is_home())) {
            p_actuator->add_correction();
            state_first_entry = true;
            return;
        }
        (*cycle_count)++;

        state_first_entry = false;

        // Calculate the waveform parameters
        if (p_waveform->calculate_waveform() == -1) {
            // Set the fault ID:
            fault_id = Fault::FT_WAVEFORM_CALC_ERROR;
            // Error in waveform calculation. Switch to error state
            set_state(States::ST_FAULT);
        }

        float goal_pos_deg = p_actuator->volume_to_degrees(C_Stat::FIFTY, p_waveparams->volume_ml / 1000);
        float vel_deg = 0;

        // Calculate how much and at what speed the actuator should move.
        p_actuator->calculate_trajectory(p_waveparams->tIn, goal_pos_deg, vel_deg);

        // Move the actuator
        p_actuator->set_position(Tick_Type::TT_DEGREES, goal_pos_deg);
        p_actuator->set_speed(Tick_Type::TT_DEGREES, vel_deg);
    }

    // Check if target has been reached.
    if (p_waveform->is_inspiration_done()) {
        set_state(States::ST_INSPR_HOLD);
    }
}

void Machine::state_inspiration_hold()
{
    if (state_first_entry) {
        state_first_entry = false;
    }
    if (p_waveform->is_inspiration_hold_done()) {
        set_state(States::ST_EXPR);
    }
}

void Machine::state_expiration()
{
    if (state_first_entry) {
        state_first_entry = false;

        float goal_pos_deg = 0;              // Fully retracted.
        float duration_s = p_waveparams->tEx;// - (now() - p_waveparams->tCycleTimer);
        float vel_deg = 0;

        // Calculate how much and at what speed the actuator should move.
        p_actuator->calculate_trajectory(duration_s, goal_pos_deg, vel_deg);

        // Move the actuator
        p_actuator->set_position(Tick_Type::TT_DEGREES, goal_pos_deg);
        p_actuator->set_speed(Tick_Type::TT_DEGREES, vel_deg);
    }

    // Check if target has been reached.
    if (p_actuator->target_reached()) {
        set_state(States::ST_PEEP_PAUSE);
    }
}

void Machine::state_peep_pause()
{
    if (state_first_entry) {
        state_first_entry = false;
    }

    if (p_waveform->is_peep_pause_done()) {
        set_state(States::ST_EXPR_HOLD);
    }
}

void Machine::state_expiration_hold()
{
    if (state_first_entry) {
        state_first_entry = false;
    }

    if (p_waveform->is_expiration_done()) {
        set_state(States::ST_INSPR);
    }
}

void Machine::state_actuator_home()
{
    // Store the is_home status temporarily.
    bool is_home = p_actuator->is_home();

    if (state_first_entry) {
        state_first_entry = false;

        // Check if the paddle is at home position
        // If not move the paddle to home.
        if (is_home) {
            // Let the motor driver know that this is 0 position
            p_actuator->set_position_as_home();

            set_state(States::ST_OFF);
        }
        else {
            // Start the home sequence
            p_actuator->home();
        }
    }

    // Has the paddle reached home? If no, keep moving.
    if (is_home) {
        // Actuator is home. Stop the actuator.
        p_actuator->set_speed(Tick_Type::TT_DEGREES, 0);
        // Let the motor driver know that this is 0 position
        p_actuator->set_position_as_home();

        set_state(States::ST_OFF);
    }
    else {
        // Homing in progress.

        /* Check if the actuator is moving, by checking feedback
        * only if home is not reached.
        * Also do the check after a time delay as it takes time for
        * the drive to respond.
        */
        if (machine_timer > check_actuator_move_in_ticks) {
            if ((is_home == false) && (p_actuator->is_moving() == false)) {
                // Set the fault ID:
                fault_id = Fault::FT_ACTUATOR_FAULT;
                // Actuator is not moving. Switch to error state
                set_state(States::ST_FAULT);
            }
        }
    }
}

void Machine::state_actuator_jog()
{
    // Stub for jogging the actuator during debug.
}

void Machine::state_fault()
{
    if (state_first_entry) {
        state_first_entry = false;

        // Stop the actuator
        p_actuator->set_speed(Tick_Type::TT_DEGREES, 0);
    }
}

void Machine::state_debug()
{
}

void Machine::state_off()
{
    if (state_first_entry) {
        state_first_entry = false;

        // Reset the cycle counter
        *cycle_count = 0;

        // Reset all alarms.
        p_alarm_manager->allOff();
    }
}

void Machine::run()
{
    // Increment the soft timer.
    machine_timer++;
    handle_errors();

    // State Machine
    switch (state) {
        case States::ST_STARTUP:
            state_startup();
            break;
        case States::ST_INSPR:
            state_inspiration();
            break;
        case States::ST_INSPR_HOLD:
            state_inspiration_hold();
            break;
        case States::ST_EXPR:
            state_expiration();
            break;
        case States::ST_PEEP_PAUSE:
            state_peep_pause();
            break;
        case States::ST_EXPR_HOLD:
            state_expiration_hold();
            break;
        case States::ST_ACTUATOR_HOME:
            state_actuator_home();
            break;
        case States::ST_ACTUATOR_JOG:
            state_actuator_jog();
            break;
        case States::ST_FAULT:
            state_fault();
            break;
        case States::ST_DEBUG:
            state_debug();
            break;
        case States::ST_OFF:
            state_off();
            break;
        default:
            break;
    }
}

void Machine::setup()
{
    // Initial state
    state = States::ST_STARTUP;
    p_alarm_manager->begin();
}

const char* Machine::get_current_state_string()
{
    return state_string[(int) state];
}

const char** Machine::get_state_list(uint8_t* size)
{
    *size = sizeof(state_string) / sizeof(char*);
    return state_string;
}

States Machine::get_current_state()
{
    return state;
}

void Machine::change_state(States st)
{
    // Change the state.
    set_state(st);
}

void Machine::handle_errors()
{
    // These pressure alarms only make sense after homing
    if (state_first_entry && state == States::ST_INSPR) {
        p_alarm_manager->badPlateau(false);
        p_alarm_manager->lowPressure(false);
        p_alarm_manager->noTidalPres(false);
    }

    p_alarm_manager->update();
}