#pragma once

#include "DaisyDuino.h"

// Unmute this if you soldered 
// the Daisy on the back of the board
// #define DAISY_IS_ON_THE_BACK

namespace synthux {
namespace simple {

enum class Analog {
    S30 = A0,
    S31 = A1,
    S32 = A2,
    S33 = A3,
    S34 = A4,
    S35 = A5,
    S36 = A6,
    S37 = A7,
    S38 = A8,
    S39 = A9,
    S40 = A10,
    S43 = A11
};

enum class Digital {
    S00 = D0,
    S01 = D1,
    S02 = D2,
    S03 = D3,
    S04 = D4,
    S05 = D5,
    S06 = D6,
    S07 = D7,
    S08 = D8,
    S09 = D9,
    S10 = D10,
    S11 = D11,
    S12 = D12,
    S13 = D13,
    S14 = D14,
    S30 = D15,
    S31 = D16,
    S32 = D17,
    S33 = D18,
    S34 = D19,
    S35 = D20,
    S36 = D21,
    S37 = D22,
    S38 = D23,
    S39 = D24,
    S40 = D25,
    S41 = D26,
    S42 = D27,
    S43 = D28,
    S44 = D29,
    S45 = D30
};

enum class AnalogOnTheBack {
    S19 = A0,
    S18 = A1,
    S17 = A2,
    S16 = A3,
    S15 = A4,
    S14 = A5,
    S13 = A6,
    S12 = A7,
    S11 = A8,
    S10 = A9,
    S09 = A10,
    S06 = A11
};

enum class DigitalOnTheBack {
    S48 = D0,
    S47 = D1,
    S46 = D2,
    S45 = D3,
    S44 = D4,
    S43 = D5,
    S42 = D6,
    S41 = D7,
    S40 = D8,
    S39 = D9,
    S38 = D10,
    S37 = D11,
    S36 = D12,
    S35 = D13,
    S34 = D14,
    S19 = D15,
    S18 = D16,
    S17 = D17,
    S16 = D18,
    S15 = D19,
    S14 = D20,
    S13 = D21,
    S12 = D22,
    S11 = D23,
    S10 = D24,
    S09 = D25,
    S08 = D26,
    S07 = D27,
    S06 = D28,
    S05 = D29,
    S04 = D30
};

template<class AP, class DP>
class Pin {
public:
  static uint8_t a(AP pin) {
    return uint8_t(pin);
  }

  static uint8_t d(DP pin) {
    return uint8_t(pin);
  }
};

using DaisyPin = Pin<Analog, Digital>; 
using OnTheBackDaisyPin = Pin<AnalogOnTheBack, DigitalOnTheBack>;

};
};

#ifndef DAISY_IS_ON_THE_BACK
#define A(p) synthux::simple::DaisyPin::a(synthux::simple::Analog::p)
#define D(p) synthux::simple::DaisyPin::d(synthux::simple::Digital::p)
#else
#define A(p) synthux::simple::OnTheBackDaisyPin::a(synthux::simple::AnalogOnTheBack::p)
#define D(p) synthux::simple::OnTheBackDaisyPin::d(synthux::simple::DigitalOnTheBack::p)
#endif
