#pragma once;

namespace synthux {

class MValue {
  public:
    MValue(): 
      _is_active    { false },
      _is_changing  { false },
      _init_value   { 0.f },
      _value        { 0.f } 
      {};

  void Init(float value) {
    _value = value;
  }

  void SetActive(bool active, float value) {
    if (active && !_is_active) {
      _init_value = value;
    }
    else if (_is_active && !active) {
      _is_changing = false;
    }
    _is_active = active;
  }

  float Process(float value) {
    if (!_is_active) return _value;
    if (!_is_changing && abs(value - _init_value) < kTreshold) return _value;
    _is_changing = true;
    _value = value;
    return _value;
  }

  bool IsChanging() {
    return _is_changing;
  }

  float Value() {
    return _value;
  }

  private:
    static constexpr float kTreshold = 0.02;

    bool _is_active;
    bool _is_changing;
    float _init_value;
    float _value;
};

};
