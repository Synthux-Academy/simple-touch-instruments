#pragma once

namespace synthux {

/**
 * @brief
 * A four milliseconds micro ASR envelope. 
 * Usable for things like muting the sound preventing a click.
 */
class SoftSwitch {
public:
  SoftSwitch(): 
  _out { 0.f },
  _stage { Stage::idle } 
  {}
  
  ~SoftSwitch() {}

  void Init(const float sample_rate) {
    _kof = 250.f / sample_rate; //4 ms
  }

  void SetOn(const bool on) {
    _on = on;
  }

  bool IsOn() {
    return _on;
  }

  float Process(const bool inverse = false) {
    switch (_stage) {
      case Stage::idle:
      _out = 0;
      if (_on) _stage = Stage::rise;
      break;

      case Stage::rise:
      if (!_on) _stage = Stage::fall;
      else if (_out >= 0.999) _stage = Stage::hold;
      else _out += (1.f - _out) * _kof;
      break;
    
      case Stage::hold:
      _out = 1.f;
      if (!_on) _stage = Stage::fall;
      break;

      case Stage::fall:
      if (_on) _stage = Stage::rise;
      else if (_out <= 0.001) _stage = Stage::idle;
      else _out -= _out * _kof; 
      break;
    }

    return inverse ? 1.f - _out : _out;
  }

private:
  SoftSwitch(const SoftSwitch &other) = delete;
  SoftSwitch(SoftSwitch &&other) = delete;
  SoftSwitch& operator=(const SoftSwitch &other) = delete;
  SoftSwitch& operator=(SoftSwitch &&other) = delete;

  enum class Stage {
    idle,
    rise,
    hold,
    fall
  };

  float _kof;
  float _out;
  Stage _stage;
  bool _on;
};

};
