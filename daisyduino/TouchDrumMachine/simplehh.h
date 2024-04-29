#pragma once;

#include "DaisyDuino.h"

namespace synthux {
  class SimpleHH {
    public:
      void Init(float sample_rate) {
        _hh.Init(sample_rate);
        _hh.SetDecay(0.7);
        _hh.SetTone(0.8);
        _hh.SetNoisiness(0.7);
      }

      float Process(bool trigger) {
        return _hh.Process(trigger);
      }

      void SetTone(float value) {
        _hh.SetDecay(0.5 + 0.5 * value);
        _hh.SetTone(0.5 + 0.5 * (1 - value));
        _hh.SetNoisiness(0.3 + 0.5 * (1 - value));
      }

    private:
      HiHat<> _hh;
  };
};