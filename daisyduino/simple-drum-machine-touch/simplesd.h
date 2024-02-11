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
    _drv.SetDrive(0.5);
  }

  float Process(bool trigger) {
    if (trigger) _osc.Reset();
    auto amp = _env.Process(trigger);
    _noise.SetAmp(amp);
    _osc.SetAmp(amp * 0.5);
    return _drv.Process(_osc.Process() + _noise.Process() * _noise_kof);
  }

  void SetTone(float value) {
    _noise_kof = (1 - value);
    _osc.SetFreq(100.f + 50.f * value);
    _env.SetTime(ADSR_SEG_RELEASE, .005f + (.1f * value));
    if (value > 0.5) _drv.SetDrive(0.05 + 0.55 * value);
  }

private:
  Adsr _env;
  WhiteNoise _noise;
  Oscillator _osc;
  Overdrive _drv;
  float _noise_kof = 1.0;
};

};
