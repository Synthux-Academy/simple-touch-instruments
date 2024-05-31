#pragma once

#include "DaisyDSP.h"
#include "env.h"

namespace synthux {

class Vox {
public:

enum class Osc2Mode {
  sound,
  am
};

Vox(): 
  _osc1_freq_mult { 1.f },
  _osc2_freq_mult { 1.f },
  _osc2_mode      { Osc2Mode::sound }
{}
~Vox() {}

void Init(float sample_rate) {
  _sample_rate = sample_rate;
  _osc1.Init(sample_rate);
  _osc1.SetWaveform(Oscillator::WAVE_SAW);
  _osc2.Init(sample_rate);
  _osc2.SetWaveform(Oscillator::WAVE_TRI);
  _env.Init(sample_rate);
}

void SetOsc1Mult(const float value) {
  _osc1_freq_mult = value;
}

void SetOsc2Mult(const float value) {
  _osc2_freq_mult = value;
}

void SetOsc2Amount(const float value) {
  _osc2_amount = value;
}

void SetOsc2Mode(const Osc2Mode mode) {
  _osc2_mode = mode;
} 

void SetOsc1Shape(const float value) {
  if (value < .5f) _osc1.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
  else _osc1.SetWaveform(Oscillator::WAVE_POLYBLEP_SQUARE);
}

void NoteOn(float freq, float amp, bool retrigger) {
  if (retrigger) {
    _env.Reset();
    _pending_freq = freq;
  }
  else {
    _base_freq = freq;
    _env.Trigger();
  }
}

void NoteOff() {
  _env.Release();
}

void SetEnvelope(const float value) {
  _env.SetShape(value);
}

void SetEnvelopeMode(const Envelope::Mode mode) {
  _env.SetMode(mode);
} 

float Process() {
  auto out = 0.f;
  auto env = _env.Process();
  if (_env.IsRunning()) {
    auto osc1_amp = env;
    auto osc2_out = _osc2.Process() * _osc2_amount;
    auto osc2_base_freq = _base_freq;
    switch (_osc2_mode) {
      case Osc2Mode::sound: 
        out = osc2_out * env;
        break;
      case Osc2Mode::am: 
        osc1_amp *= (1.f - _osc2_amount * (1 - osc2_out)) * 1.6 + 0.9 * _osc2_amount;
        osc2_base_freq = 5.f;
        break;
    }
    _osc1.SetFreq(_osc1_freq_mult * _base_freq);
    _osc2.SetFreq(_osc2_freq_mult * osc2_base_freq);

    out += _osc1.Process() * osc1_amp;
  }
  else if (_pending_freq > 0) {
    _base_freq = _pending_freq;
    _pending_freq = 0;
    _env.Trigger();
  }
  return out * .75f;
}

private:
  static constexpr float kSlopeMin  = 0.01f;
  static constexpr float kSlopeMax  = 1.99f;

  Oscillator _osc1;
  Oscillator _osc2;
  synthux::Envelope _env;
  float _sample_rate;
  float _base_freq;
  float _pending_freq;
  float _osc1_freq_mult;
  float _osc2_freq_mult;
  float _osc2_amount;
  Osc2Mode _osc2_mode;
  bool _is_pending;
};
};
