#ifndef UVENT_FAULT_H
#define UVENT_FAULT_H

enum class Fault {
    FT_NONE,
    FT_ACTUATOR_FAULT,
    FT_ACTUATOR_INVALID_TIME,
    FT_WAVEFORM_CALC_ERROR,
};

#endif