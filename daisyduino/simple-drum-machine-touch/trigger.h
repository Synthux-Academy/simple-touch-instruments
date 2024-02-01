#pragma once

namespace synthux {

enum class Every {
  _4th  = 1,
  _8th  = 2,
  _16th = 4,
  _32th = 8
};

template<uint16_t ppqn, Every resolution = Every::_16th>
class Trigger {
public:
    Trigger():
    _swing         { 0 },
    _swing_kof     { 0 },
    _swing_on      { false },
    _iterator      { 0 },
    _next_trigger     { 0 },
    _trigger_count    { 0 },
    _odd_count     { 0 },
    _odd_count_max { 0 },
    _is_odd        { false }
    {
      if (resolution == Every::_16th || resolution == Every::_32th) {
        _swing_on = true;
        _swing_kof = 5.f * static_cast<float>(ppqn) / 48.f; //48ppqn is a reference
        _odd_count_max = static_cast<uint8_t>(resolution) / 4;
      }
    }

    void SetSwing(const float frac_swing) {
        // For 48 ppqn. Conversion to actual ppqn is
        // considered in _swing_kof ////////////////
        // |  0  |  1  |  2  |  3  |  4  |  5  |
        // | 50% | 54% | 58% | 62% | 66% | 70% |
        if (_swing_on) {
          _swing = static_cast<uint16_t>(round(frac_swing * _swing_kof));
        }
    }

    bool Tick() {
        auto should_trigger = (_iterator == _next_trigger);
        if (++_iterator == kPulsesPerBar) _iterator = 0;

        if (should_trigger) {
          if (++_trigger_count == kTriggersPerBar) _trigger_count = 0;
          _next_trigger = _trigger_count * kPulsesPerTrigger;

          if (_swing_on && ++_odd_count == _odd_count_max) {
            _is_odd = !_is_odd;
            if (_is_odd) _next_trigger += _swing;
            _odd_count = 0;
          }
        }
        
        return should_trigger;
    }

    void Reset() {
      _iterator = 0;
      _trigger_count = 0;
      _next_trigger = 0;
      _is_odd = false;
      _odd_count = 0;
    }

private:
    static constexpr uint16_t kPulsesPerBar = ppqn * 4;
    static constexpr uint16_t kPulsesPerTrigger = ppqn / static_cast<size_t>(resolution);
    static constexpr uint16_t kTriggersPerBar = 4 * static_cast<size_t>(resolution);
    float _swing_kof;
    uint16_t _iterator;
    uint16_t _trigger_count;
    uint16_t _next_trigger;
    uint16_t _swing;
    uint8_t _odd_count;
    uint8_t _odd_count_max;
    bool _is_odd;
    bool _swing_on;
};

};
