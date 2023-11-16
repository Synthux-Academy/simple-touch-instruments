#pragma once;

#include "DaisyDuino.h"

namespace synthux {
class Modulator {

public:
  void Init(float sample_rate) {
    _osc.Init(sample_rate);
    _osc.SetWaveform(Oscillator::WAVE_TRI);
    SetFrequency(0.5);
  }

  void SetFrequency(float freq) {
    _osc.SetFreq(freq * 10 + 2);
  }

  void Process() {
    _val = _osc.Process();
  }

  float Value() {
    return _val;
  }

private:
  Oscillator _osc;
  float _val;
};
};
