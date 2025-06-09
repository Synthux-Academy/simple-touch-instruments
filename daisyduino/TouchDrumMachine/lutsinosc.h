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
    _freq_kof         { 0 },
    _phase            { 0 },
    _phase_increment  { 0 },
    _phase_offset     { 0 }
    {}

    ~LUTSinOsc() {}

    void Init(float sample_rate) {
      _freq_kof = static_cast<float>(kSize) / sample_rate;
    }

    void SetFreq(const float freq) {
        _phase_increment = freq * _freq_kof;
    }

    void SetPhaseOffset(const float value) {
      _phase_offset = static_cast<float>(kSize) * value;
    }

    void Reset() {
      _phase = 0;
    }

    float Process() {
      auto size = static_cast<float>(kSize);
      auto phase = _wrap_min_max(_phase + _phase_offset, 0.f, size);

      //Do linear interpollation a + k * (b - a)
      auto _int_phase = static_cast<size_t>(phase);
      auto _frac_phase = phase - _int_phase;
      auto _next_phase = _int_phase + 1;
      while (_next_phase >= kSize) _next_phase -= kSize;
      auto sample = _lut.table[_int_phase] + _frac_phase * (_lut.table[_next_phase] - _lut.table[_int_phase]);

      //Advance phase
      _phase += _phase_increment;
      while (_phase >= size) _phase -= size;
      
      return sample;
    }

private:
    LUTSinOsc(const LUTSinOsc &other) = delete;
    LUTSinOsc(LUTSinOsc &&other) = delete;
    LUTSinOsc& operator=(const LUTSinOsc &other) = delete;
    LUTSinOsc& operator=(LUTSinOsc &&other) = delete;

    // https://stackoverflow.com/questions/4633177/how-to-wrap-a-float-to-the-interval-pi-pi
    float _wrap_max(const float x, const float max) {
      return fmodf(max + fmodf(x, max), max);
    }
    float _wrap_min_max(const float x, const float min, const float max) {
      return min + _wrap_max(x - min, max - min);
    }

    float _freq_kof;
    float _phase;
    float _phase_increment;
    float _phase_offset;

    static constexpr size_t kSize = 512;
    static constexpr LUTSin<kSize> _lut = LUTSin<kSize>();
};

};
