#pragma once
#include <array>

namespace synthux {
class Scale {
public:
  Scale() : _scale_index{0} {}

  void SetScaleIndex(uint8_t index) { _scale_index = index; }

  float FreqAt(uint8_t idx) { return _scales[_scale_index][idx]; }

private:
  uint8_t _scale_index;
  std::array<std::array<float, 8>, 3> _scales = {{
      {146.83, 164.81, 185.00, 196.00, 246.94, 293.66, 329.63,
       369.99}, // D Pygmy
      {164.81, 220.00, 233.08, 277.18, 293.66, 329.63, 349.23,
       392.00}, // E Hijaz
      {146.83, 164.81, 174.61, 220.00, 246.94, 293.66, 329.63,
       349.23} // D dorean pentatonic
  }};
};
}; // namespace synthux
