#include <cstdlib>
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
    _portamento = fmax(portamento * portamento, 0.05) * 0.0005;  
  }
}

void SetFreq(float pitch) {
  _is_on = (pitch > 1.f);
  _targetFreq = pitch;
  _is_gliding = true;
}

float Process() {
    if (!_is_on) return 0;
    if (_is_gliding) {
      auto delta = (_targetFreq - _oscFreq);
      if (fabs(delta) < 0.1f) {
        _oscFreq = _targetFreq;
        _is_gliding = false;
      }
      else {
        _oscFreq += delta * _portamento;
      }
    }
    _osc.SetFreq(_oscFreq * (1.f + _lfo.Process()));
    return _osc.Process();
}

private:
  Oscillator _osc;
  Oscillator _lfo;
  float _oscFreq;
  float _targetFreq;
  float _portamento;
  bool _is_on;
  bool _is_gliding;
};

};