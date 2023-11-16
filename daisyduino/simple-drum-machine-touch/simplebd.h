#pragma once

#include "DaisyDuino.h"

namespace synthux {

class SimpleBD {
public:

  void Init(float sample_rate) {
    _noise.Init();
    
    _amp_env.Init(sample_rate);
    _amp_env.SetTime(ADSR_SEG_ATTACK, .0);
    _amp_env.SetTime(ADSR_SEG_DECAY, .01);
    _amp_env.SetTime(ADSR_SEG_RELEASE, .04);
    _amp_env.SetSustainLevel(0.8);

    _freq_env.Init(sample_rate);
    _freq_env.SetTime(ADSR_SEG_ATTACK, .0);
    _freq_env.SetTime(ADSR_SEG_DECAY, .001);
    _freq_env.SetTime(ADSR_SEG_RELEASE, .005);
    _freq_env.SetSustainLevel(0.0);

    _osc.Init(sample_rate);
    _osc.SetWaveform(Oscillator::WAVE_SIN);
    _osc.SetFreq(50.0);
  }

  float Process(bool trigger) {
    if (trigger) {
      _osc.Reset();
      gate_counter = kGateCounterMax;
    }
    auto is_gate_open = gate_counter > 0;
    auto amp = _amp_env.Process(is_gate_open);
    auto freq = _freq_env.Process(trigger);
    if (is_gate_open) gate_counter--;
    _noise.SetAmp(amp);
    _osc.SetFreq(50.0 + freq * 1000.0);
    _osc.SetAmp(amp);
    return _osc.Process() + _noise.Process() * 0.03;
  }

private:
  static constexpr uint32_t kGateCounterMax = 2880; //60ms @ 48K
  uint32_t gate_counter = 0;

  Adsr _amp_env;
  Adsr _freq_env;
  WhiteNoise _noise;
  Oscillator _osc;
};

};
