#pragma once
#include <array>
#include <math.h>

namespace synthux {

template<size_t size>
struct LUTSin {
  std::array<float, size> table;
  constexpr LUTSin(): table() {
    for (size_t i = 0; i < size; i++) {
        table[i] = std::clamp(sinf(2.f * M_PI * static_cast<float>(i) / static_cast<float>(size)), -1.f, 1.f); 
    }
  }
};

class LUTSinOsc {
public:
    LUTSinOsc():
    _freq_kof     { 0 },
    _phase        { 0 },
    _phase_delta  { 0 }
    {}

    ~LUTSinOsc() {}

    void Init(float sample_rate) {
      _freq_kof = static_cast<float>(kSize) / sample_rate;
    }

    void SetFreq(const float freq) {
        _phase_delta = freq * _freq_kof;
    }

    void Reset() {
      _phase = 0;
    }

    float Process() {
      //Do linear interpollation a + k * (b - a)
      auto _int_phase = static_cast<size_t>(_phase);
      auto _frac_phase = _phase - _int_phase;
      auto _next_phase = _int_phase + 1;
      while (_next_phase >= kSize) _next_phase -= kSize;
      auto sample = _lut.table[_int_phase] + _frac_phase * (_lut.table[_next_phase] - _lut.table[_int_phase]);

      //Advance phase
      _phase += _phase_delta;
      while (_phase >= kSize) _phase -= kSize;
      
      return sample;
    }

private:
    LUTSinOsc(const LUTSinOsc &other) = delete;
    LUTSinOsc(LUTSinOsc &&other) = delete;
    LUTSinOsc& operator=(const LUTSinOsc &other) = delete;
    LUTSinOsc& operator=(LUTSinOsc &&other) = delete;

    float _freq_kof;
    float _phase;
    float _phase_delta;

    static constexpr size_t kSize = 512;
    static constexpr LUTSin<kSize> _lut = LUTSin<kSize>();
};

};
