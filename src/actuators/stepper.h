#ifndef UVENT_STEPPER_H
#define UVENT_STEPPER_H

#include <cstdint>
#include "../uvent_conf.h"

class Stepper {
public:
    void init();
    bool run();
    void set_speed(float);
    // double get_position();
    void home();
    bool is_moving();
    bool remaining_steps_to_go();
    void set_position(double);
    void set_position_relative(double steps);
    void set_position_as_home(int32_t position);
    bool target_reached();
    void set_enable(bool en);
    uint32_t get_current_position();

private:
    bool enabled = false;
};

#endif