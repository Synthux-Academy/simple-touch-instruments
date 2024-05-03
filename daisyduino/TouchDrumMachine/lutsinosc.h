#pragma once
#include <array>
#include <math.h>

namespace synthux {

template<size_t size>
static constexpr std::array<float, size> sineLUT() {
    std::array<float, size> lut { 0 };
    for (int i = 0; i < size; i++) {
        lut[i] = sinf(2.f * M_PI * static_cast<float>(i) / static_cast<float>(size)); 
    }
    return lut;
  }

template<size_t size = 512>
class LUTSinOsc {
public:
    LUTSinOsc():
    _f_kof        { 0 },
    _sample_rate  { 0 },
    _phase        { 0 },
    _phase_delta  { 0 } 
    {}

    ~LUTSinOsc() {}

    void Init(float sample_rate) {
        _sample_rate = sample_rate;
        _f_kof = static_cast<float>(_lut.size()) / sample_rate;
    }

    void SetFreq(float freq) {
        _phase_delta = freq * _f_kof;
    }

    void Reset() {
      _phase = 0;
    }

    float Process() {
      auto _int_phase = static_cast<uint32_t>(_phase);
      auto _frac_phase = _phase - _int_phase;
      auto _next_phase = (_int_phase + 1) % _lut.size();
      auto s = _lut[_phase] + _frac_phase * (_lut[_next_phase] - _lut[_phase]);
      _phase += _phase_delta;
      if (_phase > _lut.size() - 1) _phase -= (_lut.size() - 1);
      return s;
    }

private:
    LUTSinOsc(const LUTSinOsc &other) = delete;
    LUTSinOsc(LUTSinOsc &&other) = delete;
    LUTSinOsc& operator=(const LUTSinOsc &other) = delete;
    LUTSinOsc& operator=(LUTSinOsc &&other) = delete;

    float _f_kof;
    float _sample_rate;
    float _phase;
    float _phase_delta;
    static constexpr std::array<float, size> _lut = { sineLUT<size>() };
};

};
