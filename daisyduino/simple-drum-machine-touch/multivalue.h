#pragma once;
#include <array>

namespace synthux {

template<uint8_t count>
class MultiValue {
  public:
    MultiValue(): 
      _active_channel { kNone },
      _is_active      { false },
      _is_changing    { false },
      _ref_value      { 0.f }
      {
        _value.fill(0.f);
      };

  void Init(std::array<float, count> value) {
    _value = value;
  }

  void SetActive(uint8_t channel, float ref_value) {
    if (channel != _active_channel) {
      _ref_value = ref_value;
      _is_changing = false;
    }
    _is_active = channel != kNone;
    _active_channel = channel;
  }

  void Process(float value) {
    if (_is_changing) {
      _value[_active_channel] = value;
    }
    else if (_is_active && abs(value - _ref_value) >= kTreshold) {
      _is_changing = true;
    }
  }

  float Value(uint8_t index) {
    return _value[index];
  }

  private:
    static constexpr float kTreshold = 0.02;
    static constexpr uint8_t kNone = 0xff;

    std::array<float, count> _value;
    float _ref_value;
    uint8_t _active_channel;
    bool _is_active;
    bool _is_changing;
};

};
