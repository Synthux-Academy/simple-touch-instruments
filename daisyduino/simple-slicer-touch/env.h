#pragma once
#include "DaisyDuino.h"

namespace synthux {

class Envelope {
public:
  void Init(float sampleRate) {
    _env.Init(sampleRate);
    _env.SetAttackTime(0.01f);
    _env.SetSustainLevel(1.f);
    _env.SetReleaseTime(0.03f);
  }

  void SetAmount(float value) {
    _env.SetReleaseTime(value);
  }

  bool IsRunning() {
    return _env.IsRunning();
  }

  float Process(bool gate) {
    return _env.Process(gate);
  }

private:
  daisysp::Adsr _env;
};

};
