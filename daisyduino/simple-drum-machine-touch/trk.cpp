#include "trk.h"
#include "DaisyDuino.h"

using namespace synthux;

Track::Track():
    _counter    { 0 },
    _slot       { 0 },
    _trigger    { nullptr },
    _is_clearing { false },
    _hit_flag { false }
    {
      _pattern.fill(0);
    }

bool Track::Tick() {
    auto slot = _slot;
    if (++_counter == kResolution) {
        _counter = 0;
        if (++_slot == _pattern.size()) _slot = 0;
        if (_is_clearing) _clear(_slot);
    }
    // As slot is changing in advance, i.e. (_slot-0.5)
    // and resolution is 2x pattern length, every second 
    // same slot value denotes onset.
    auto trigger = _hit_flag || (slot == _slot && _pattern[_slot]);
    _hit_flag = false;

    return trigger;
}

void Track::SetTriggerCallback(void(*val)()) {
    _trigger = val;
}

void Track::Hit() {
    _pattern[_slot] = true;
    _hit_flag = true;
}

void Track::_clear(uint8_t slot) {
    _pattern[slot] = false;
}

void Track::SetClearing(bool val) {
    _is_clearing = val;
}
