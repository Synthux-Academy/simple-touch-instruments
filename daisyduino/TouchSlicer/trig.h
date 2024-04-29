#pragma once

namespace synthux {

template<uint32_t ppqn>
class Trigger {
public:
    Trigger():
    _swing      { 0 },
    _iterator   { 0 },
    _next_tick  { 0 },
    _tick_count { 0 },
    _is_odd     { false }
    {}

    void SetSwing(const float frac_swing) {
        // |  0  |  1  |  2  |  3  |  4  |  5  |
        // | 50% | 54% | 58% | 62% | 66% | 70% |
        _swing = static_cast<uint32_t>(round(frac_swing * 5));
    }

    bool Tick() {
        bool trigger = false;
        if (_iterator == _next_tick) {
          trigger = true;
          if (++_tick_count == 16) _tick_count = 0;
          _is_odd = !_is_odd;
          _next_tick = _tick_count * kPP16N;
          if (_is_odd) _next_tick += _swing;
        }
        // Advance iterator
        if (++_iterator == kPPWN) _iterator = 0;

        return trigger;
    }

    void Reset() {
      _iterator = 0;
      _tick_count = 0;
      _next_tick = 0;
      _is_odd = false;
    }

private:
    static constexpr uint32_t kPPWN = ppqn * 4; // pulses per whole note
    static constexpr uint32_t kPP16N = ppqn / 4; // pulses per 16th note

    uint32_t _iterator;
    uint32_t _tick_count;
    uint32_t _next_tick;
    uint32_t _swing;
    bool _is_odd;
};

};
