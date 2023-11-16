#pragma once

#include <array>

namespace synthux {

template<uint8_t out_count>
class MultiKnob {
public:
  MultiKnob(std::array<uint16_t, out_count> init_values): 
  _is_selector_pin_high { true },
  _current_out { 0 },
  _recent_value { 0xffff },
  _sensitivity { 3 },
  _values { init_values }
  { }

  void SetSensitivity(uint16_t value) {
    _sensitivity = value;
  }

  void Process(uint16_t knob_value, bool selector_pin_high) {
    if (selector_pin_high && !_is_selector_pin_high) {
      if (++_current_out == out_count) _current_out = 0;
    }
    _is_selector_pin_high = selector_pin_high;
    if (_recent_value == 0xffff) _recent_value = knob_value;
    
    if (abs(knob_value - _recent_value) > _sensitivity) {
      _values[_current_out] = knob_value;      
      _recent_value = knob_value;
    }
  }

  uint16_t ValueAt(uint8_t index) {
    if (index >= out_count) return 0;
    return _values[index];
  }

private:
  bool _is_selector_pin_high;

  uint8_t _current_out;
  uint16_t _recent_value;
  uint16_t _sensitivity;
  std::array<uint16_t, out_count> _values;
};

};
