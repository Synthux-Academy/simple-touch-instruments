#pragma once

#include "Adafruit_MPR121.h"
#include <array>

namespace synthux {
  class Terminal {
    public:
      Terminal():
        _state { 0 },
        _on_tap { nullptr },
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
      void SetOnTap(void(*on_tap)(uint16_t pad)) {
        _on_tap = on_tap;
      }

      void SetOnRelease(void(*on_release)(uint16_t pad)) {
        _on_release = on_release;
      }

      bool IsTouched(uint16_t pad) {
        return _state & (1 << pad);
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
            if (_on_tap != nullptr && is_touched && !was_touched) {
              _on_tap(i);
            }
            else if (_on_release != nullptr && was_touched && !is_touched) {
              _on_release(i);
            }
          }
          _state = state;
      }

    private:
      void(*_on_tap)(uint16_t pad);
      void(*_on_release)(uint16_t pad);

      Adafruit_MPR121 _cap;
      uint16_t _state;
  };
};
