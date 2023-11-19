#pragma once
#include <inttypes.h>
#include <array>

namespace synthux {
class Track {
public:
    Track();
    ~Track() {};

    bool Tick();
    void Hit();
    void SetRecording(bool value) { _is_recording = value; }
    void SetClearing(bool value) { _is_clearing = value; };
    void SetSound(float value) { _snd = value; };
    float Sound();
    void Reset();

private:
    void _clear(uint8_t);
    void(*_trigger)();

    static constexpr uint8_t kResolution = 2;
    static constexpr uint8_t kPatternLength = 16;

    std::array<bool, kPatternLength> _pattern;
    std::array<float, kPatternLength> _sound;
    float _snd;
    uint8_t _counter;
    uint8_t _slot;
    bool _is_recording;
    bool _is_clearing;
};
};
