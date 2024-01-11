#pragma once
#include <stdint.h>
#include "Arduino.h"

// Derived from DaisyDuino/libDaisy AnalogControl.

namespace synthux {

class AKnob {
  public:
    AKnob(uint8_t pin,
          float coeff = 0.2,
          float quant = 200.f,
          bool  flip = false,
          bool  invert = false):
          val_    { 0.0f },
          pin_    { pin },
          coeff_  { coeff },
          quant_  { quant },
          flip_   { flip },
          invert_ { invert } 
          {};

  void Init() {
    pinMode(pin_, INPUT);
    val_ = _read();
  }

    float Process() {
      float t = _read();
      if (flip_) t = 1.f - t;
      if (invert_) t = -t;
      val_ += coeff_ * (t - val_);
      return round(val_ * quant_) / quant_;
    }

  private:
    static constexpr float kFrac = 1.f / 1023.f;

    float _read() {
      return static_cast<float>(analogRead(pin_)) * kFrac;
    }

    uint8_t pin_;
    float coeff_; 
    float val_;
    float quant_;
    bool  flip_;
    bool  invert_;
};
};
