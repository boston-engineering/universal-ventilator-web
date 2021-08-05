#include "stepper.h"
#include "../uvent_conf.h"

/* The 5718L stepper moves counter-clockwise for positive speeds
 * and clockwise for negative speeds.
 */

// Initialize the stepper motor.
void Stepper::init()
{

}

// Service the Accelstepper driver.
bool Stepper::run()
{
    return true;
}

// Run the stepper at a set speed
void Stepper::set_speed(float steps_per_second)
{
}

void Stepper::home()
{
}

bool Stepper::is_moving()
{
    return enabled;
}

bool Stepper::remaining_steps_to_go()
{
    return 0;
}

void Stepper::set_position(double steps)
{

}

void Stepper::set_position_relative(double steps)
{

}

void Stepper::set_position_as_home(int32_t position)
{

}

bool Stepper::target_reached()
{
    int32_t distance_to_go = 0;
    return (distance_to_go == 0);
}

void Stepper::set_enable(bool en)
{
    enabled = en;
}

uint32_t Stepper::get_current_position()
{
    return 0;
}