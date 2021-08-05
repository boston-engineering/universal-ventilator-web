/**
 *
 * New Tone driver for SDL adapted from
 * https://stackoverflow.com/questions/10110905/simple-sound-wave-generator-with-sdl-in-c
 * https://web.archive.org/web/20120313055436/http://www.dgames.org/beep-sound-with-sdl/
 *
 */

#ifndef UVENT_SDL_TONE_DRIVER_H
#define UVENT_SDL_TONE_DRIVER_H

#include <cstdint>
#include <queue>
#include <cmath>

const int AMPLITUDE = 28000;
const int FREQUENCY = 44100;

struct BeepObject {
    double freq;
    int samplesLeft;
};

class Beeper {
private:
    double v;
    std::queue<BeepObject> beeps;
public:
    Beeper();

    ~Beeper();

    void beep(double freq, int duration);

    void generateSamples(uint16_t *stream, int length);

    void wait();
};

void audio_callback(void *, uint8_t *, int);

#endif //UVENT_SDL_TONE_DRIVER_H
