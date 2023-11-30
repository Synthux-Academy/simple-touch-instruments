#pragma once
#include <array>

namespace synthux {

class Driver {
public:
  static constexpr size_t kVoices = 4;

  Driver(): 
    _base_freq    { _scale[1][0] },
    _spread       { 0 },
    _scale_index  { 1 }
  {}
  ~Driver() = default;

  void SetPitchAndSpread(float pitch, float spread, bool quantized) {
    auto& s = _scale[_scale_index];
    _base_freq = quantized ? s[static_cast<size_t>(pitch * 17.f)] : _map(pitch, s[0], s[s.size() - 1]);
    _spread = spread;
    if (quantized) _SetFreqQuantized();
    else _SetFreqFree();
  }

  void SetScaleIndex(size_t index) {
    _scale_index = index;
  }

  float FreqAt(size_t i) {
    return _freqs[i];
  }

private:
  void _SetFreqFree() {
    auto fifth = 1.5f;
    auto octav = 2.0f;
    _freqs[0] = _base_freq;
    _freqs[1] = _Freq(_base_freq, 1.5f);
    _freqs[2] = _Freq(_base_freq, 2.0f);
    _freqs[3] = _Freq(_freqs[2], 1.5f);
  }

  void _SetFreqQuantized() {
    _freqs[0] = _base_freq;
    auto index = (_chords.size() - 1) * _spread;
    _freqs[1] = _chords[index][0] * _base_freq;
    _freqs[2] = _chords[index][1] * _base_freq;
    _freqs[3] = _chords[index][2] * _base_freq;
  }

  float _map(float val, float min, float max) {
    return (max - min) * val + min;
  }

  float _Freq(float baseFreq, float kof) {
    return ((kof - 1.f) * _spread + 1.f) * baseFreq;
  }

  static constexpr float kFifth = 1.5f;
  static constexpr float kOctav = 2.0f;

  std::array<float, kVoices> _freqs;
  float _base_freq;
  float _spread;
  size_t _scale_index;

  std::array<std::array<float, 18>, 3> _scale = {{
    { 65.41, 73.42, 82.41, 98.00, 110.00, 130.81, 146.83, 164.81, 196.00, 220.00, 261.63, 293.66, 329.63, 392.0, 440.00, 523.25, 587.33, 659.26 },
    { 73.42, 82.41, 87.31, 110.0, 123.47, 146.83, 164.81, 174.61, 220.00, 246.94, 293.66, 329.63, 349.23, 440.0, 493.88, 587.33, 659.26, 698.46 },
    { 87.31, 98.00, 110.00, 130.81, 146.83, 174.61, 196.00, 220.00, 261.63, 293.66, 349.23, 392.00, 440.00, 523.25, 587.33, 698.46, 783.99, 880.00 }
  }};

  std::array<std::array<float, 3>, 7> _chords = {{
    { 1.0, 1.0, 1.0 }, //none
    { 1.25, 1.5, 1.0 }, //maj
    { 1.2, 1.5, 1.0 },  //min
    { 1.33333, 1.5, 1.0 }, //sus4
    { 1.25, 1.5, 1.8 }, //7th
    { 1.2, 1.8, 1.0 }, //min7
    { 1.25, 1.875 , 2.25 } //maj9
  }};
};

};
