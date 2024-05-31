#pragma once
#include "DaisyDSP.h"
#include "env.h"

namespace synthux {

class Filter {
public:
  Filter(): 
  _freq       { 0.f },
  _env_amount { 1.f }
  {}
  ~Filter() {}

  void Init(const float sample_rate) {
    _env.Init(sample_rate);
    _flt.Init(sample_rate);
    _flt.SetFreq(10000.f);
    _flt.SetRes(0.2f);
  }

  void Trigger(bool retrigger = false) {
    if (retrigger) {
      _env.Reset();
      _pending_retrigger = true;
    }
    else {
      _env.Trigger();
    }
  }

  void Release() {
    _env.Release();
  }

  void SetEnvelope(const float value) {
    _env.SetShape(value);
  }

  void SetEnvelopeMode(const Envelope::Mode mode) {
    _env.SetMode(mode);
  } 

  void SetEnvelopeAmount(const float value) {
    _env_amount = value;
  }

  void SetFreq(const float value) {
    _freq = fmap(value, kFMin, kFMax);
    _freq_env_room = 10000.f - _freq;
  }

  void SetReso(const float value) {
    _flt.SetRes(fmap(value, 0.f, .9f));
  }

  float Process(const float in) {
    auto env = _env.Process() * _env_amount;
    if (_env.IsRunning()) {
      auto freq = min(_freq + _freq_env_room * env, kFMax);
      _flt.SetFreq(freq);
      _flt.Process(in);
    }
    else if (_pending_retrigger) {
      _pending_retrigger = false;
      _env.Trigger();
    }
    return _flt.Low();
  }

private:
  static constexpr float kFMin = 40.f;
  static constexpr float kFMax = 10000.f;

  Svf _flt;
  Envelope _env;
  float _freq;
  float _freq_env_room;
  float _env_amount;
  bool _pending_retrigger;
};

};
