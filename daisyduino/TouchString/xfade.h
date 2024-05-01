#pragma once

namespace synthux {

// The square law crossfade
// adopted from Will. C. Pirkle "Designing Software Synthesizer Plugins in C++" book.
class XFade {
public:
  XFade():
    _lhs { 1.f },
    _rhs { 0.f } 
    {}

  void Process(const float lhs0, const float lhs1, const float rhs0, const float rhs1, float& out0, float& out1) {
    out0 = lhs0 * _lhs + rhs0 * _rhs;
    out1 = lhs1 * _lhs + rhs1 * _rhs;
  }

  void SetStage(const float value) {
    auto sq = value * value;
    _lhs = 1.f - sq;
    _rhs = 2.f * value - sq;
  }

private:
  float _lhs;
  float _rhs;
};
};