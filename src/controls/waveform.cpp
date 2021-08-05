#include <complex>
#include "../utilities/util.h"
#include "waveform.h"

waveform_params* Waveform::get_params(void)
{
    return &params;
}

int8_t Waveform::calculate_waveform()
{
    // Store the current time
    params.tCycleTimer = lv_tick_get();

    // Seconds in each breathing cycle period
    params.tPeriod = 60 / params.bpm;

    // Inspiration hold time
    params.tHoldIn = params.tPeriod / (1.0 + (params.ie_i / params.ie_e));

    params.tIn = params.tHoldIn - HOLD_IN_DURATION;
    if (params.tIn < 0.0) {
        // tIn Cannot be negative.
        return -1;
    }

    params.tEx = std::min(params.tHoldIn + MAX_EX_DURATION, params.tPeriod - MIN_PEEP_PAUSE);

    return 0;
}

bool Waveform::is_inspiration_done()
{
    return ((now_s() - params.tCycleTimer) > params.tIn);
}

bool Waveform::is_inspiration_hold_done()
{
    return ((now_s() - params.tCycleTimer) > params.tHoldIn);
}

bool Waveform::is_expiration_done()
{
    return ((now_s() - params.tCycleTimer) > params.tPeriod);
}

bool Waveform::is_peep_pause_done()
{
    return ((now_s() - params.tCycleTimer) > (params.tEx + MIN_PEEP_PAUSE));
}

void Waveform::display_details() const
{
    printf("----Waveform Details----\n");
    printf("tPeriod:\t %0.2f\n", params.tPeriod);
    printf("tHoldIn:\t %0.2f\n", params.tHoldIn);
    printf("tIn:\t\t %0.2f\n", params.tIn);
    printf("tEx:\t\t %0.2f\n", params.tEx);
    printf("bpm:\t\t %d\n", params.bpm);
    printf("ie:\t\t %0.1f:%0.1f\n", params.ie_i, params.ie_e);
    printf("Vt:\t\t %0.1f\n", params.volume_ml);
    printf("pip:\t\t %d\n", params.pip);
    printf("peep:\t\t %d\n", params.peep);
}