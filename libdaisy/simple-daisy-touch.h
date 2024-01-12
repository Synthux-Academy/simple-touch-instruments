#pragma once

#include "daisy.h"
#include "daisy_seed.h"
#include <array>

using namespace daisy;
using namespace seed;

namespace synthux {
namespace simpletouch {

///////////////////////////////////////////////////////////////
//////////////////////// TOUCH SENSOR /////////////////////////

class Touch {
public:
  Touch() : _state{0}, _on_touch{nullptr}, _on_release{nullptr} {}

  void Init(DaisySeed hw) {
    // Uncomment if you want to use i2C4
    // Wire.setSCL(D13);
    // Wire.setSDA(D14);
    _hw = hw;
    Mpr121I2C::Config config;

    int result = _cap.Init(config);

    if (result != 0) {
      _hw.PrintLine("MPR121 config failed");
    }
  }

  // Register note on callback
  void SetOnTouch(void (*on_touch)(uint16_t pad)) { _on_touch = on_touch; }

  void SetOnRelease(void (*on_release)(uint16_t pad)) {
    _on_release = on_release;
  }

  bool IsTouched(uint16_t pad) { return _state & (1 << pad); }

  bool HasTouch() { return _state > 0; }

  void Process() {
    uint16_t pad;
    bool is_touched;
    bool was_touched;
    auto state = _cap.Touched();
    for (uint16_t i = 0; i < 12; i++) {
      pad = 1 << i;
      is_touched = state & pad;
      was_touched = _state & pad;
      if (_on_touch != nullptr && is_touched && !was_touched) {
        _on_touch(i);
      } else if (_on_release != nullptr && was_touched && !is_touched) {
        _on_release(i);
      }
    }
    _state = state;
  }

private:
  uint16_t _state;
  void (*_on_touch)(uint16_t pad);
  void (*_on_release)(uint16_t pad);
  Mpr121I2C _cap;
  DaisySeed _hw;
};

///////////////////////////////////////////////////////////////
//////////////////////////// PINS /////////////////////////////
// Probably can simplify this since Analog and Digital share pins
namespace Analog {
static constexpr Pin S30 = A0;
static constexpr Pin S31 = A1;
static constexpr Pin S32 = A2;
static constexpr Pin S33 = A3;
static constexpr Pin S34 = A4;
static constexpr Pin S35 = A5;
static constexpr Pin S36 = A6;
static constexpr Pin S37 = A7;
}; // namespace Analog

namespace Digital {
static constexpr Pin S07 = D6;
static constexpr Pin S08 = D7;
static constexpr Pin S09 = D8;
static constexpr Pin S10 = D9;
static constexpr Pin S30 = D15;
static constexpr Pin S31 = D16;
static constexpr Pin S32 = D17;
static constexpr Pin S33 = D18;
static constexpr Pin S34 = D19;
static constexpr Pin S35 = D2;
}; // namespace Digital

template <class AP, class DP> class PinST {
public:
  static int a(AP pin) { return int(pin); }

  static int d(DP pin) { return int(pin); }
};

// using DaisyPin = PinST<Analog, Digital>;

}; // namespace simpletouch
}; // namespace synthux
