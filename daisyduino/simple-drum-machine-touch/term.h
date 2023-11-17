#pragma once

#include "Adafruit_MPR121.h"
#include <array>

namespace synthux {
  class Terminal {
    public:
      Terminal():
        _state { 0 },
        _on_tap { nullptr }
        {
          
        }

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
      void SetOnTap(void(*on_tap)(uint8_t pad)) {
        _on_tap = on_tap;
      }

      void Process() {
          uint16_t pin;
          bool is_touched;
          bool was_touched;
          auto state = _cap.touched();
        
          for (uint16_t i = 0; i < 12; i++) {
            pin = 1 << i;
            is_touched = state & pin;
            was_touched = _state & pin;
            if (is_touched && !was_touched) {
              _on_tap(i);
            }
          }
          _state = state;
      }

    private:
      void(*_on_tap)(uint8_t pad);

      Adafruit_MPR121 _cap;
      uint16_t _state;
  };
};
