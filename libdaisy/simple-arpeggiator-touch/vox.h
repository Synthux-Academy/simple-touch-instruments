#pragma once

#include "../randomrange.h"
#include "daisysp.h"

using namespace daisysp;

namespace synthux {

class Vox {
public:
  void Init(float sample_rate) {
    // OSCILLATOR SETUP
    _osc.Init(sample_rate);
    _osc.SetWaveform(Oscillator::WAVE_TRI);

    // LFO SETUP
    _lfo.Init(sample_rate);

    _lfo.SetFreq(_rnd.generate(50, 150) / 10.f);
    _lfo.SetWaveform(Oscillator::WAVE_TRI);
    _lfo.SetAmp(0.005);

    // ENV SETUP
    _env.Init(sample_rate);
    _env.SetTime(ADSR_SEG_ATTACK, .02);
    _env.SetTime(ADSR_SEG_RELEASE, .02);
  }

  void NoteOn(float freq, float amp) {
    _osc_freq = freq;
    _osc_amp = amp;
    OpenGate();
  }

  void NoteOff() { CloseGate(); }

  float Process() {
    auto env_amp = _env.Process(_gate);
    if (!_env.IsRunning())
      return 0;
    _osc.SetFreq(_osc_freq * (1.f + _lfo.Process()));
    _osc.SetAmp(env_amp * _osc_amp);
    return _osc.Process();
  }

private:
  void OpenGate() { _gate = true; }

  void CloseGate() { _gate = false; }

  bool _gate = false;
  float _osc_freq;
  float _osc_amp;
  Oscillator _osc;
  Oscillator _lfo;
  Adsr _env;
  Randomrange _rnd;
};
}; // namespace synthux
