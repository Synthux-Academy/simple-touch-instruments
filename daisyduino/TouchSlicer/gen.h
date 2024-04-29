#pragma once;
#include <array>
#include "buf.h"

namespace synthux {

enum class SliceShape {
  ASD, // short attack, long sustain, short decay
  AD //short attack, long decay
};

class Slice;

static constexpr size_t kMaxLength = 72000; //1.5s @ 48K
static constexpr size_t kMinLength = 960; //20ms @ 48K
static constexpr size_t kRange = kMaxLength - kMinLength;

template<int positions>
class Generator {
public:
  void Init(Buffer* buffer) {
      _buffer = buffer;
      for (auto& s: _slices) s.Init(buffer);  
  }

  void SetPosition(size_t i, float value) {
    _positions[i] = static_cast<size_t>(kRange * value + kMinLength);
  }

  void SetSpeed(float speed) {
    _speed = 2 * speed;    
  }

  void SetReverse(bool reverse) {
    _reverse = reverse;
  }

  void SetShape(float shape) {
    if (shape > 0.5) {
      _slice_length = kMinLength + 2.f * shape * kRange - kRange;
      _shape = SliceShape::ASD;
    }
    else {
      _slice_length = kMaxLength - 2.f * shape * kRange;
      _shape = SliceShape::AD;
    }
  }

  std::array<size_t, positions>& MakeSlices() {
    size_t slice = _buffer->Length() / positions;
    for (size_t i = 0; i < positions; i++) {
      _positions[i] = i * slice;
    }
    return _positions;
  }

  void Activate(int position_index) {
    for (auto& s: _slices) {
      if (!s.IsActive()) {
        auto speed = _reverse ? -_speed : _speed;
        s.Activate(_positions[position_index], _slice_length, _shape, speed);
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
  Buffer* _buffer;
  std::array<size_t, positions> _positions;
  std::array<Slice, 2 * positions> _slices;
  size_t _slice_length = kMaxLength / 2;
  float _speed;
  SliceShape _shape;
  bool _reverse;
  
};
///////////////////////////////////////////////////////////////
///////////////////////// SLICE ///////////////////////////////
class Slice {
public:
  void Init(Buffer* buffer) {
    _buf = buffer;
  }

  bool IsActive() {
    return _is_active;
  }

  void Activate(size_t position, size_t length, SliceShape shape, float speed) {
    _play_head = speed > 0 ? 0 : length - 1;
    _iterator = 0;
    _speed = speed;
    _position = position;
    _length = length;
    _decay_start = (shape == SliceShape::ASD) ? length - kASDFadeOut : kFadeIn + 1;
    _decay_kof = 1.f / static_cast<float>(length - _decay_start);
    _stage = ShapeStage::attack;
    _is_active = true;
  }

  void Process(float& out0, float& out1) {
    if (_iterator == _length) {
      out0 = 0;
      out1 = 0;
      _is_active = false;
      return;
    }
    
    auto int_ph = static_cast<int32_t>(_play_head);
    auto frac_ph = _play_head - int_ph;
    auto next_ph = _speed < 0 ? int_ph - 1 : int_ph + 1;

    int_ph += _position;
    next_ph += _position;
    if (int_ph < 0) int_ph += _buf->Length();
    if (next_ph < 0) next_ph += _buf->Length();

    auto a0 = 0.f;
    auto a1 = 0.f;
    auto b0 = 0.f;
    auto b1 = 0.f;
    _buf->Read(int_ph, a0, a1);
    _buf->Read(next_ph, b0, b1);
    
    auto att = _Attenuation();
    out0 = (a0 + frac_ph * (b0 - a0)) * att;
    out1 = (a1 + frac_ph * (b1 - a1)) * att;

    _iterator ++;
    _play_head += _speed;
  }

private:

  float _Attenuation() {
    switch (_stage) {
      case ShapeStage::attack: 
        if (_iterator == kFadeIn) _stage = ShapeStage::sustain;
        return static_cast<float>(_iterator) / static_cast<float>(kFadeIn);
        
      case ShapeStage::sustain:
        if (_iterator == _decay_start) _stage = ShapeStage::decay;
        return 1.0;

      case ShapeStage::decay:
        return static_cast<float>(_length - _iterator - 1) * _decay_kof;
    }
  }

  static constexpr size_t kFadeIn = kMinLength / 2;
  static constexpr size_t kASDFadeOut = kMinLength / 2;

  enum class ShapeStage {
    attack,
    sustain,
    decay
  };

  Buffer* _buf;
  float _play_head;
  float _speed;
  float _decay_kof;
  size_t _decay_start;
  size_t _position;
  size_t _length;
  size_t _iterator;
  ShapeStage _stage;
  bool _is_active = false;
};

};