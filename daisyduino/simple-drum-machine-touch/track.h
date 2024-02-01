#pragma once
#include <inttypes.h>
#include <array>

namespace synthux {
class Track {
public:
    Track():
    _counter        { 0 },
    _slot           { 0 },
    _last_hit_slot  { 0xff },
    _trigger        { nullptr },
    _is_clearing    { false }
    {
      _pattern.fill(0);
      _automation.fill(0.5);
    }
    ~Track() {};

    bool Tick() {
        auto slot = _slot;
        if (++_counter == kResolution) {
            _counter = 0;
            if (++_slot == _pattern.size()) _slot = 0;
            if (_is_clearing) _clear(_slot);
        }
        
        // As slot is changing in advance, i.e. (_slot-0.5)
        // and resolution is 2x pattern length, every second 
        // same slot value denotes onset.
        auto tick = (slot == _slot && _pattern[_slot] && _slot != _last_hit_slot);
        _last_hit_slot = 0xff;
        return tick;
    }

    float AutomationValue() {
      return _automation[_slot];
    }

    void HitStroke(float automation_value) {
      if (_is_recording) {
        _pattern[_slot] = true;
        _automation[_slot] = automation_value;
        _last_hit_slot = _slot;
      }
    }

    void SetRecording(bool value) { 
      _is_recording = value; 
    }

    void SetClearing(bool value) {
      _is_clearing = value;
    }

    void Reset() {
      _slot = 0;
      _counter = 0;
    }

private:
    void _clear(uint8_t slot) {
        _pattern[slot] = false;
    }
    
    void(*_trigger)();

    static constexpr uint8_t kResolution = 2;
    static constexpr uint8_t kPatternLength = 16;

    std::array<bool, kPatternLength> _pattern;
    std::array<float, kPatternLength> _automation;
    uint8_t _counter;
    uint8_t _slot;
    uint8_t _last_hit_slot;
    bool _is_recording;
    bool _is_clearing;
};
};
