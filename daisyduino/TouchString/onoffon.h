#pragma once;

namespace synthux {

class OnOffOn {
public:
  OnOffOn(const int pin_a, const int pin_b):
  _pin_a { pin_a },
  _pin_b { pin_b } 
  {}
  ~OnOffOn() {};

  void Init() const {
    pinMode(_pin_a, INPUT_PULLUP);
    pinMode(_pin_b, INPUT_PULLUP);
  };

  uint32_t Value() const {
    auto a_val = static_cast<uint32_t>(digitalRead(_pin_a));
    auto b_val = static_cast<uint32_t>(!digitalRead(_pin_b));
    return a_val + b_val;
  }

private:
  int _pin_a;
  int _pin_b;
};
};
