#pragma once

#include "DaisyDuino.h"

namespace synthux {

class SimpleSD {
public:

  void Init(float sample_rate) {
    _noise.Init();
    _env.Init(sample_rate);
    _env.SetTime(ADSR_SEG_ATTACK, .0);
    _env.SetTime(ADSR_SEG_RELEASE, .03);
    _osc.Init(sample_rate);
    _osc.SetWaveform(Oscillator::WAVE_TRI);
    _osc.SetFreq(100.f);
  }

  float Process(bool trigger) {
    if (trigger) _osc.Reset();
    auto amp = _env.Process(trigger);
    _noise.SetAmp(amp);
    _osc.SetAmp(amp * 0.5);
    return _osc.Process() + _noise.Process();
  }

private:
  Adsr _env;
  WhiteNoise _noise;
  Oscillator _osc;
};

};
