#include <cstring>
#include <cstdio>

#include "storage.h"

void Storage::get_settings(uvent_settings& outset)
{
    memcpy(&outset, &def_settings, sizeof(uvent_settings));
}

void Storage::display_storage()
{
    uvent_settings temp_set{};
    get_settings(temp_set);

    printf("---- UVENT SETTINGS ---\n");
    printf("Serial no.: %s\n", temp_set.serial);
    printf("Diff. pressure Type: %d\n", temp_set.diff_pressure_type);
    printf("Actuator home offset: %d\n", temp_set.actuator_home_offset_adc_counts);
    printf("Tidal Volume: %d\n", temp_set.tidal_volume);
    printf("Resp. Rate: %d\n", temp_set.respiration_rate);
    printf("PEEP: %d\n", temp_set.peep_limit);
    printf("PIP: %d\n", temp_set.pip_limit);
    printf("Plateau: %d\n", temp_set.plateau_time);
    printf("IE: %.1f : %.1f\n", temp_set.ie_ratio_left, temp_set.ie_ratio_right);
}