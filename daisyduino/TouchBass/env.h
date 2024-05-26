#pragma once
#include <algorithm>

namespace synthux {

class Envelope {
public:
  enum class Mode {
    AR,
    ASR
  };

  Envelope():
  _curve_kof { .5f },
  _phase { 0 },
  _out { 0.f },
  _stage { Stage::idle },
  _mode { Mode::AR }
  {}

  void Init(const float sample_rate) {
    _t_range_2x = 2.0 * 4.f * sample_rate; // 4s. Multiplication by 2 is done for optimisation
    _t_min_attack = .01f * sample_rate; // 20 ms
    _t_min_decay_a = .4f * sample_rate; // 400 ms
    _t_min_decay_b = .01f * sample_rate; // 10 ms
    _t_reset = .008f * sample_rate; // 8 ms
    _t_reset_kof = 1.f / _t_reset;
  }

  bool IsRunning() {
    return _stage != Stage::idle;
  }

  void SetMode(const Mode mode) {
    _mode = mode;
  }

  void SetShape(const float value) {
    float curve;
    if (value < 0.5) {
      _t_attack = _t_min_attack;
      _t_decay = static_cast<size_t>(_t_range_2x * value * value) + _t_min_decay_a;
      curve = value * .15f;
    }
    else {
      auto norm_val = 2.f * value - 1.f;
      _t_attack = static_cast<size_t>(_t_range_2x * norm_val * norm_val) + _t_min_attack;
      _t_decay = static_cast<size_t>(_t_range_2x * (1.f - value * value)) + _t_min_decay_b;
      curve = .5f;
    }
    _t_attack_kof = 1.f / _t_attack;
    _t_decay_kof = 1.f / _t_decay;
    _set_curve(curve);
  }
  
  void Trigger() {
    switch (_stage) {
      case Stage::idle:
        _stage = Stage::attack;
        _phase = 0;
        break;

      case Stage::decay:
        _stage = Stage::attack;
        _phase = _ph_attack(_out);
        break;

      default: break;
    }
  }

  void Release() {
    switch (_stage) {
      case Stage::attack:
        _phase = _ph_decay(_out);
        _stage = Stage::decay;
        break;

      case Stage::sustain:
        _phase = 0;
        _stage = Stage::decay;
        break;

      default: break;
    }
  }

  float Process() {
    float ph;
    switch (_stage) {
      case Stage::idle: 
        _out = 0.f;
        break;

      case Stage::attack: 
        ph = static_cast<float>(_phase) * _t_attack_kof;
        _out = _amp_attack(ph);
        if (_phase >= _t_attack) {
          _stage = _mode == Mode::AR ? Stage::decay : Stage::sustain;
          _phase = 0;
        }
        else {
          _phase ++;
        }
        break;

      case Stage::sustain:
        _out = 1.f;
        break;

      case Stage::decay:
        ph = static_cast<float>(_phase) * _t_decay_kof;
        _out = _amp_decay(ph);
        if (_phase >= _t_decay) {
          _stage = Stage::idle;
          _phase = 0;
        }
        else {
          _phase ++;
        }
        break;

      case Stage::reset:
        _out = _t_reset_out - static_cast<float>(_phase) * _t_reset_kof;
        if (_phase >= _t_reset) {
          _stage = Stage::idle;
          _phase = 0;
        }
        else {
          _phase ++;
        }
        break;
    }
    return std::min(std::max(_out, 0.f), 1.f);
  }

  void Reset() {
    if (_stage == Stage::idle) return;
    _stage = Stage::reset;
    _t_reset_out = _out;
    _phase = 0;
  }

private:
  enum class Stage {
    idle,
    attack,
    sustain,
    decay,
    reset
  };

  void _set_curve(const float value) {
    auto cu = value - .5f;
    _curve_kof = 128.f * cu * cu;
  }

  // Derived from Stages by Emilie Gillet
  float _amp_attack(const float ph) {
    return ph / (1.f + _curve_kof * (1.f - ph));
  }
  float _amp_decay(const float ph) {
    return (1.f - ph) / (1.f + _curve_kof * ph);
  }
  size_t _ph_attack(float amp) {
    return static_cast<size_t>(roundf(_t_attack * amp * (1 + _curve_kof) / (1 + amp * _curve_kof)));
  }
  size_t _ph_decay(float amp) {
    return static_cast<size_t>(roundf(_t_decay * (1 - amp) / (amp * _curve_kof + 1)));
  }
  
  float _out;
  float _curve_kof;
  float _t_min_attack;
  float _t_min_decay_a;
  float _t_min_decay_b;
  float _t_range_2x;
  float _t_reset_out;
  float _t_reset;
  float _t_reset_kof;
  float _t_attack_kof;
  float _t_decay_kof;
  size_t _t_attack;
  size_t _t_decay;
  size_t _phase;
  Mode _mode;
  Stage _stage;
  bool has_changed;
};
};
