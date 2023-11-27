#pragma once
#include "DaisyDuino.h"

namespace synthux {

class Envelope {
public:
  void Init(float sampleRate) {
    _env.Init(sampleRate);
  }

  void SetAmount(float value) {
    _env.SetAttackTime(5.f * value);
    _env.SetSustainLevel(1.f);
    _env.SetReleaseTime(0.5 * value);
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
