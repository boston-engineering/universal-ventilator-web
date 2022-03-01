#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include "util.h"

bool has_time_elapsed(uint32_t* timer_ptr, uint32_t n)
{
    uint32_t ms = lv_tick_get();
    bool result = ms - *timer_ptr >= n;

    if (ms < *timer_ptr || result) {
        *timer_ptr = ms;
        return true;
    }

    return false;
}

bool is_whole(double x, double epsilon)
{
    return abs(x - floor(x)) < epsilon;
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
    return data;
}