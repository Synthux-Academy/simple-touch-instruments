#include "trk.h"
#include "DaisyDuino.h"

using namespace synthux;

Track::Track():
    _counter    { 0 },
    _slot       { 0 },
    _trigger    { nullptr },
    _is_clearing { false }
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
    return (slot == _slot && _pattern[_slot]);
}

float Track::Sound() {
  return _sound[_slot];
}

void Track::Hit() {
  if (_is_recording) {
    _pattern[_slot] = true;
    _sound[_slot] = _snd;
  }
}

void Track::_clear(uint8_t slot) {
    _pattern[slot] = false;
}

void Track::Reset() {
  _slot = 0;
  _counter = 0;
}