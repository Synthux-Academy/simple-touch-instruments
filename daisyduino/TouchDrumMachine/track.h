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
        // Remember current slot
        auto slot = _slot;
        // The slot is adwanced every two ticks, 
        // and one tick before actual onset position.
        if (++_counter == 2) {
            _counter = 0;
            // Advance and wrap the current slot.
            if (++_slot == _pattern.size()) _slot = 0;
            // Clear slot if in clearing mode.
            if (_is_clearing) _clear(_slot);
        }
        
        // If the slot was not advanced 
        // it means we're at the onset possion,
        // and if this position is not empty
        // in the pattern array - the drum should be triggered.
        // A check against _last_hit_slot is 
        // to prevent double triggering during recording.
        auto should_trigger = (slot == _slot && _pattern[_slot] && _slot != _last_hit_slot);
        // Reset last hit slot.
        _last_hit_slot = 0xff;
        
        return should_trigger;
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
    void _clear(uint32_t slot) {
        _pattern[slot] = false;
    }
    
    void(*_trigger)();

    static constexpr uint32_t kTicksPerSlot = 2;
    static constexpr uint32_t kPatternLength = 16;

    std::array<bool, kPatternLength> _pattern;
    std::array<float, kPatternLength> _automation;
    uint32_t _counter;
    uint32_t _slot;
    uint32_t _last_hit_slot;
    bool _is_recording;
    bool _is_clearing;
};
};
