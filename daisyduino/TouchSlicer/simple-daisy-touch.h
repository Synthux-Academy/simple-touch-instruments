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

#ifdef TEST_PADS
void testPads() {
  for (auto i = 0; i < 12; i++) {
    if (touch.IsTouched(i)) {
      Serial.print("IS TOUCHED ");
      Serial.println(i);
    }
  }
}
#endif

#ifdef TEST_KNOBS
void testKnobs() {
  Serial.print("S30: ");
  Serial.print(analogRead(knob_a));
  Serial.print(" S31: ");
  Serial.print(analogRead(knob_b));
  Serial.print(" S32: ");
  Serial.print(analogRead(knob_c));
  Serial.print(" S33: ");
  Serial.print(analogRead(knob_d));
  Serial.print(" S34: ");
  Serial.print(analogRead(knob_e));
  Serial.print(" S35: ");
  Serial.print(analogRead(knob_f));
  Serial.print(" Fader L: ");
  Serial.print(analogRead(left_fader));
  Serial.print(" Fader R: ");
  Serial.print(analogRead(right_fader));
  Serial.println("");
}
#endif

#ifdef TEST_SWITCHES
void testSwitches() {
  Serial.print("S7: ");
  Serial.print(digitalRead(switch_1_a));
  Serial.print(" S8: ");
  Serial.print(digitalRead(switch_1_b));
  Serial.print(" S9: ");
  Serial.print(digitalRead(switch_2_a));
  Serial.print(" S10: ");
  Serial.print(digitalRead(switch_2_b));
  Serial.println("");
}
#endif
