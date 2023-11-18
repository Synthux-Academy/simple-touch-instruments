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
    void SetClearing(bool);
    void SetTriggerCallback(void(*)());
    void SetSound(float value);

private:
    void _clear(uint8_t);
    void(*_trigger)();

    static constexpr uint8_t kResolution = 2;
    static constexpr uint8_t kPatternLength = 16;

    std::array<bool, kPatternLength> _pattern;
    std::array<float, kPatternLength> _sound;
    uint8_t _counter;
    uint8_t _slot;
    bool _is_clearing;
};
};
