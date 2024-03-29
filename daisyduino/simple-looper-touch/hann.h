#pragma once

namespace synthux {

template<size_t win_slope>
class Hann {
  private:
  static constexpr std::array<float, win_slope> hannCurve() {
    std::array<float, win_slope> slope { 0 };
    for (int i = 0; i < win_slope; i++) {
      auto sin = std::sin(HALFPI_F * static_cast<float>(i) / static_cast<float>(win_slope - 1));
      slope[i] = sin * sin;
    }
    return slope;
  }

  public:
  static constexpr std::array<float, win_slope> curve { hannCurve() };
  static constexpr float kSlopeX2 = 2.f * win_slope - 1; 

  static float fmap_symmetric(const float value) {
    return curve[static_cast<size_t>(kSlopeX2 * abs(value - 0.5f))];
  }
};
};
