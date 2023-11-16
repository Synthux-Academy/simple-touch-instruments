#pragma once

#include "DaisyDuino.h"

namespace synthux {

class Click {
public:

  void Init(float sample_rate) {
    _env.Init(sample_rate);
    _env.SetTime(ADSR_SEG_ATTACK, .0);
    _env.SetTime(ADSR_SEG_RELEASE, .01);
    _env.SetSustainLevel(1.0);
    _osc.Init(sample_rate);
    _osc.SetWaveform(Oscillator::WAVE_TRI);
    _osc.SetFreq(100.0);
  }

  float Process(bool trigger) {
    auto amp = _env.Process(trigger);
    _osc.SetAmp(amp);
    return _osc.Process();
  }

private:
  Adsr _env;
  Oscillator _osc;
};
};
