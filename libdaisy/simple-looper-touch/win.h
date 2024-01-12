#pragma once
#include <array>

namespace synthux {

template<size_t length>
constexpr std::array<float, length> Slope() {
    std::array<float, length> slope { 0 };
    for (unsigned int i = 0; i < length; i++) slope[i] = static_cast<float>(i) / static_cast<float>(length - 1);
    return slope;
}

template<size_t win_slope>
class Window {
public:
    Window():
    _play_head    { 0 },
    _delta        { 0 },
    _loop_start   { 0 },
    _iterator     { 0 },
    _is_active    { false }
    {}

    void Activate(float start, float delta, size_t loop_start) {
        _play_head = start;
        _delta = delta;
        _loop_start = loop_start;
        _iterator = 0;
        _is_active = true;
    }

    bool IsActive() { return _is_active; }

    void Deactivate() {
      _is_active = false;
    }

    bool IsHalf() {  return _iterator == kHalf;  }

    float PlayHead() { return _play_head; }

    void Process(Buffer* buf, float& out0, float& out1) {
        auto int_ph = static_cast<size_t>(_play_head);
        auto frac_ph = _play_head - int_ph;
        auto next_ph = _delta > 0 ? int_ph + 1 : int_ph - 1;

        auto a0 = 0.f;
        auto a1 = 0.f;
        auto b0 = 0.f;
        auto b1 = 0.f;
        buf->Read(int_ph + _loop_start, a0, a1);
        buf->Read(next_ph + _loop_start, b0, b1);
        
        auto att = _Attenuation();
        out0 = (a0 + frac_ph * (b0 - a0)) * att;
        out1 = (a1 + frac_ph * (b1 - a1)) * att;

        _play_head += _delta;
        if (_play_head < 0) {
          _play_head += buf->Length();
        }
        if (++_iterator == kSize) _is_active = false;
    }
  
private:
    float _Attenuation() {
      auto idx = (_iterator < kHalf) ? _iterator : kSize - _iterator - 1;
      return kSlope[idx];
    }

    static constexpr size_t kHalf { win_slope };
    static constexpr size_t kSize { 2 * win_slope };
    static constexpr std::array<float, win_slope> kSlope { Slope<win_slope>() };

    float _play_head;
    float _delta;
    size_t _loop_start;
    size_t _iterator;
    bool _is_active;
};

};
