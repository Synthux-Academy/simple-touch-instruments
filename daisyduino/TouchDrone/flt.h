#pragma once

#include "DaisyDuino.h"

namespace synthux {

class Filter {
public:
  void Init(float sampleRate) {
    _flt.Init(sampleRate);
  }

  void SetTimbre(float timbre) {
    auto fltFreq = _map(timbre, 80.f, 16000.f);
    auto fltRes = 1.f - _map(timbre, 0.3f, 1.0f);
    _flt.SetFreq(fltFreq);
    _flt.SetRes(fltRes); 
  }

  float Process(const float in) {
    return _flt.Process(in);
  }

private:
  daisysp::MoogLadder _flt;

  float _map(float val, float min, float max) {
    return (max - min) * val + min;
  }
};

};
