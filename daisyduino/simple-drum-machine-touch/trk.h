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

private:
    void _clear(uint8_t);
    void(*_trigger)();

    static constexpr uint8_t kResolution = 2;
    uint8_t _counter;
    std::array<bool, 16> _pattern;
    uint8_t _slot;
    uint8_t _is_clearing;
    bool _hit_flag;
};
};
