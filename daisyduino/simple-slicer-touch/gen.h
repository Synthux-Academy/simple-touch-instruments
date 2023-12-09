#include <cstddef>
#pragma once;
#include <array>
#include "buf.h"
#include "env.h"

namespace synthux {

class Slice;

template<int positions>
class Generator {
public:
  void Init(Buffer* buffer, float sample_rate) {
      _buffer = buffer;
      for (auto& s: _slices) s.Init(buffer, sample_rate);  
  }

  void SetPosition(size_t i, float value) {
    _positions[i] = static_cast<size_t>((kMaxLength - kMinLength) * value + kMinLength);
  }

  void SetSpeed(float speed) {
    _speed = 2 * speed;    
  }

  void SetShape(float shape) {
    if (shape > 0.5) {
      _slice_length = 2.f * (shape - 0.475) * kMaxLength;
      _release_time = 0.03f;
    }
    else {
      _slice_length = kMinLength;
      _release_time = 2.f * (0.5 - shape) + 0.03f; 
    }
  }

  std::array<size_t, positions>& MakeSlices() {
    size_t slice = _buffer->Length() / positions;
    for (size_t i = 0; i < positions; i++) {
      _positions[i] = i * slice;
    }
    return _positions;
  }

  void Activate(int slice_index) {
    for (auto& s: _slices) {
      if (!s.IsActive()) {
        s.Activate(_positions[slice_index], _slice_length, _release_time, _speed);
        return;
      }
    }
  };

  void Process(float& out0, float& out1) {
    out0 = 0;
    out1 = 0;
    auto s_out0 = 0.f;
    auto s_out1 = 0.f;
    for (auto& s: _slices) {
      if (s.IsActive()) {
        s.Process(s_out0, s_out1);
        out0 += s_out0;
        out1 += s_out1;
      }
    }
  }

private:
  static constexpr size_t kMaxLength = 36000;
  static constexpr size_t kMinLength = 240;
  Buffer* _buffer;
  std::array<size_t, positions> _positions;
  std::array<Slice, 2 * positions> _slices;
  float _speed;
  size_t _slice_length = kMaxLength / 2;
  float _release_time;
};

class Slice {
public:
  void Init(Buffer* buffer, float sample_rate) {
    _buf = buffer;
    _env.Init(sample_rate);
  }

  bool IsActive() {
    return _is_active;
  }

  void Activate(size_t position, size_t length, float release, float delta) {
    _play_head = 0;
    _delta = delta;
    _position = position;
    _length = length;
    _env.SetAmount(release);
    _is_active = true;
  }

  void Process(float& out0, float& out1) {
    auto int_ph = static_cast<size_t>(_play_head);
    auto vol = _env.Process(int_ph < _length);
    if (!_env.IsRunning()) {
      out0 = 0;
      out1 = 0;
      _is_active = false;
      return;
    }
    
    auto frac_ph = _play_head - int_ph;
    auto next_ph = int_ph + 1;

    auto a0 = 0.f;
    auto a1 = 0.f;
    auto b0 = 0.f;
    auto b1 = 0.f;
    _buf->Read(int_ph + _position, a0, a1);
    _buf->Read(next_ph + _position, b0, b1);
    
    out0 = (a0 + frac_ph * (b0 - a0)) * vol;
    out1 = (a1 + frac_ph * (b1 - a1)) * vol;
  
    _play_head += _delta;
  }

private:
  Buffer* _buf;
  Envelope _env;
  float _delta;
  float _play_head;
  size_t _position;
  size_t _length;
  bool _is_active = false;
};

};