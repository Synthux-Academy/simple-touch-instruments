#include <algorithm>
#pragma once

#include "Adafruit_MPR121.h"
#include <array>

namespace synthux {
  class Terminal {
    public:
      Terminal():
        _state { 0 },
        _latch { false } {
          _hold.fill(false);
          _is_touched.fill(false);
        }

      void Init() {
        // Uncomment if you want to use i2C4
        // Wire.setSCL(D13);
        // Wire.setSDA(D14);s
        
        if (!_cap.begin(0x5A)) {
          Serial.println("MPR121 not found, check wiring?");
          while (1) {
            Serial.println("PLEASE CONNECT MPR121 TO CONTINUE TESTING");
            delay(200);
          }
        }
      }

      bool IsOn(uint8_t index) {
        return _hold[index];
      }

      bool IsTouched(uint8_t index) {
        return _is_touched[index];
      }

      bool IsLatched() {
        return _latch;
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
            //On touch 
            _is_touched[i] = is_touched;
            if (is_touched && !was_touched) {  
              if (i == 3 || i == 4 || i == 6) {
                if (_latch && _hold[i]) _SetOff(i);
                else _SetOn(i);
                continue;
              }
              
              //Latch pin 3
              // if (i == 2) {
              //   _latch = !_latch;
              //   if (!_latch) _SetAllOff();
              //   continue;    
              // }
            }
            //On release
            else if (!_latch && !is_touched && was_touched) _SetOff(i);
          }
          _state = state;
      }

    private:
      void _SetOn(uint16_t i) {
        _hold[i] = true;
      }

      void _SetOff(uint16_t i) {
        _hold[i] = false;
      }

      void _SetAllOff() {
        _hold.fill(false);
      }

      static constexpr uint8_t kTracksCount = 3;

      Adafruit_MPR121 _cap;
      uint16_t _state;
      std::array<bool, 12> _is_touched;
      std::array<bool, 12> _hold;
      bool _latch;
  };
};
