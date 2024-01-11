#pragma once
#include <array>

namespace synthux {

class Scale {
public:
  Scale() {};
  ~Scale() {};

  float FreqAt(uint8_t i) {
    return _scale[i];
  }

private:
  std::array<float, 12> _scale = { 65.41, 73.42, 82.41, 98.00, 110.00, 130.81, 146.83, 164.81, 196.00, 220.00, 261.63, 293.66 };
};

};
