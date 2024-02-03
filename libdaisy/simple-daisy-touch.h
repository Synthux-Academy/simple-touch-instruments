#pragma once

#include "daisy.h"
#include "daisy_seed.h"
#include <array>

using namespace daisy;
using namespace seed;

using namespace daisy;

namespace synthux {
namespace simpletouch {

enum AdcChannel {
  S30 = 0,
  S31,
  S32,
  S33,
  S34,
  S35,
  S36,
  S37,
  ADC_LAST
};

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

    /** ADC Init */
    AdcChannelConfig adc_config[ADC_LAST];
    /** Order of pins to match enum expectations */
    dsy_gpio_pin adc_pins[] = {
      A0,
      A1,
      A2,
      A3,
      A4,
      A5,
      A6,
      A7,
    };

    for(int i = 0; i < ADC_LAST; i++)
    {
        adc_config[i].InitSingle(adc_pins[i]);
    }
    _hw.adc.Init(adc_config, ADC_LAST);
    _hw.adc.Start();

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

  float GetAdcValue(int idx) { return _hw.adc.GetFloat(idx); }

private:
  uint16_t _state;
  void (*_on_touch)(uint16_t pad);
  void (*_on_release)(uint16_t pad);
  Mpr121I2C _cap;
  DaisySeed _hw;
};


}; // namespace simpletouch
}; // namespace synthux
