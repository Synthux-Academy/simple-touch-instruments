#pragma once

#include "Adafruit_MPR121.h"
#include <array>

namespace synthux {
  class Terminal {
    public:
      static constexpr uint16_t kNotesCount = 8;

      Terminal():
        _state { 0 },
        _latch { false },
        _on_note_on { nullptr },
        _on_note_off { nullptr }
        {
          _hold.fill(false);
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
      void SetOnNoteOn(void(*on_note_on)(uint8_t num, uint8_t vel)) {
        _on_note_on = on_note_on;
      }

      // Register note off callback
      void SetOnNoteOff(void(*on_note_off)(uint8_t num)) {
        _on_note_off = on_note_off;
      }

      void SetOnScaleSelect(void(*on_scale_select)(uint8_t num)) {
        _on_scale_select = on_scale_select;
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
            if (is_touched && !was_touched) {
              //Notes pins 0-7
              if (i < kNotesCount) {
                if (_latch && _hold[i]) _SetOff(i); 
                else _SetOn(i);  
                continue;
              }
              //Latch pin 8
              if (i == 8) {
                _latch = !_latch;
                if (!_latch) _SetAllOff(state);
                continue;    
              }
              
              //Scale select pin 9-11
              // if (i > 8) _on_scale_select(i - 9);
            }
            //On release
            else if (!_latch && !is_touched && was_touched) _SetOff(i);
          }

          _state = state;
      }

    private:
      void _SetOn(uint16_t i) {
        _on_note_on(i, 127);
        _hold[i] = true;
      }

      void _SetOff(uint16_t i) {
        _on_note_off(i);
        _hold[i] = false;
      }

      void _SetAllOff(uint16_t state) {
        for (uint16_t i = 0; i < _hold.size(); i++) {
          if (_hold[i] && !(state & 1 << i)) _on_note_off(i);
        }
        _hold.fill(false);
      }

      void(*_on_note_on)(uint8_t num, uint8_t vel);
      void(*_on_note_off)(uint8_t num);
      void(*_on_scale_select)(uint8_t index);

      Adafruit_MPR121 _cap;
      uint16_t _state;
      std::array<bool, kNotesCount> _hold;
      bool _latch;
  };
};
