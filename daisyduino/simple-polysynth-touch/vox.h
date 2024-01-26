#pragma once

#include "DaisyDuino.h"

namespace synthux {

class Vox {
public:
void Init(float sample_rate) {
  _osc.Init(sample_rate);
  _osc.SetWaveform(Oscillator::WAVE_SIN);
  _env.Init(sample_rate);
  _env.SetAttackTime(0.1f);
  _env.SetSustainLevel(1.f);
  _env.SetReleaseTime(0.3f);
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
  _targetFreq = pitch;
  _is_gliding = true;
}

float Process(bool gate) {
    auto vol = _env.Process(gate);
    if (!_env.IsRunning()) return 0;

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
    _osc.SetFreq(_oscFreq);
    return _osc.Process() * vol;
}

private:
  daisysp::Oscillator _osc;
  daisysp::Adsr _env;
  float _oscFreq;
  float _targetFreq;
  float _portamento = 1.f;
  bool _is_gliding;
};
};