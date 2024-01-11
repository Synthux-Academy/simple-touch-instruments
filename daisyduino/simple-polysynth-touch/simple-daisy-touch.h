#pragma once

#include "DaisyDuino.h"
#include "Adafruit_MPR121.h"
#include <array>

namespace synthux {
namespace simpletouch {

///////////////////////////////////////////////////////////////
//////////////////////// TOUCH SENSOR /////////////////////////

class Touch {
  public:
    Touch():
      _state { 0 },
      _on_touch { nullptr },
      _on_release { nullptr }
      {}

    void Init() {
      // Uncomment if you want to use i2C4
      // Wire.setSCL(D13);
      // Wire.setSDA(D14);
      
      if (!_cap.begin(0x5A)) {
        Serial.println("MPR121 not found, check wiring?");
        while (1) {
          Serial.println("PLEASE CONNECT MPR121 TO CONTINUE TESTING");
          delay(200);
        }
      }
    }

    // Register note on callback
    void SetOnTouch(void(*on_touch)(uint16_t pad)) {
      _on_touch = on_touch;
    }

    void SetOnRelease(void(*on_release)(uint16_t pad)) {
      _on_release = on_release;
    }

    bool IsTouched(uint16_t pad) {
      return _state & (1 << pad);
    }

    bool HasTouch() {
      return _state > 0;
    }

    void Process() {
        uint16_t pad;
        bool is_touched;
        bool was_touched;
        auto state = _cap.touched();
        for (uint16_t i = 0; i < 12; i++) {
          pad = 1 << i;
          is_touched = state & pad;
          was_touched = _state & pad;
          if (_on_touch != nullptr && is_touched && !was_touched) {
            _on_touch(i);
          }
          else if (_on_release != nullptr && was_touched && !is_touched) {
            _on_release(i);
          }
        }
        _state = state;
    }

  private:
    void(*_on_touch)(uint16_t pad);
    void(*_on_release)(uint16_t pad);

    Adafruit_MPR121 _cap;
    uint16_t _state;
};

///////////////////////////////////////////////////////////////
//////////////////////////// PINS /////////////////////////////

enum class Analog {
    S30 = A0,
    S31 = A1,
    S32 = A2,
    S33 = A3,
    S34 = A4,
    S35 = A5,
    S36 = A6,
    S37 = A7
};

enum class Digital {
    S07 = D6,
    S08 = D7,
    S09 = D8,
    S10 = D9,
    S30 = D15,
    S31 = D16,
    S32 = D17,
    S33 = D18,
    S34 = D19,
    S35 = D20
};

template<class AP, class DP>
class Pin {
public:
  static int a(AP pin) {
    return int(pin);
  }

  static int d(DP pin) {
    return int(pin);
  }
};

using DaisyPin = Pin<Analog, Digital>;

};
};

#define A(p) synthux::simpletouch::DaisyPin::a(synthux::simpletouch::Analog::p)
#define D(p) synthux::simpletouch::DaisyPin::d(synthux::simpletouch::Digital::p)
