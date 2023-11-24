#pragma once

#include "DaisyDuino.h"

namespace synthux {

class Vox {
public:
void Init(float sample_rate) {
  _osc.Init(sample_rate);
  _osc.SetWaveform(Oscillator::WAVE_SAW);

  _lfo.Init(sample_rate);
  _lfo.SetFreq(random(50, 150) / 10.0);
  _lfo.SetWaveform(Oscillator::WAVE_TRI);
  _lfo.SetAmp(0.002f);
}

void SetPortamento(float portamento) {
  if (portamento > 0.995) {
    _portamento = 1.f;
  }
  else {
    _portamento = fmax(portamento, 0.2) * 0.0005;  
  }
}

void SetFreq(float pitch) {
  _targetFreq = pitch;
}

float Process() {
    _oscFreq += (_targetFreq - _oscFreq) * _portamento;
    _osc.SetFreq(_oscFreq * (1.f + _lfo.Process()));
    return _osc.Process();
}

private:
  Oscillator _osc;
  Oscillator _lfo;
  float _oscFreq;
  float _targetFreq;
  float _portamento;
};

};