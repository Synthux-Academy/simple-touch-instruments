#pragma once

#include "DaisyDuino.h"
#include "lutsinosc.h"

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
    _osc.SetFreq(_base_freq);
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
    _osc.SetFreq(_base_freq + freq * 1000.f);
    return _osc.Process() * amp + _noise.Process() * _noise_level;
  }

  void SetTone(float value) {
    _noise_level = 0.07f * (1 - value);
    _base_freq = 40.f + 20.f * (1 - value);
    _amp_env.SetTime(ADSR_SEG_RELEASE, .005f + (.1f * value));
  }

private:
  static constexpr uint32_t kGateCounterMax = 2880; //60ms @ 48K
  uint32_t gate_counter = 0;

  Adsr _amp_env;
  Adsr _freq_env;
  WhiteNoise _noise;
  LUTSinOsc _osc;
  float _noise_level = .03f;
  float _base_freq = 50.f;
};

};
