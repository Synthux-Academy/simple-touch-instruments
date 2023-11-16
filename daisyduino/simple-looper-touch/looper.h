#include "utility/DaisySP/modules/dsp.h"
#include "WSerial.h"
#include "mod.h"
#pragma once

#include "buf.h"
#include "mod.h"
#include "win.h"
#include <array>

namespace synthux {

template<size_t win_slope = 192>
class Looper {
  public:
    Looper():
    _buffer             { nullptr },
    _delta              { 1.f },
    _direction          { 1.f },
    _rand_amount        { 0 },
    _volume             { 1.f },
    _release_kof        { 0.f },
    _loop_start         { 0 },
    _loop_start_offset  { 0 },
    _loop_length        { 0 },
    _last_playhead      { 0 },
    _is_playing         { false },
    _is_gate_open       { false },
    _mode               { Mode::loop }
    {}

    void Init(Buffer* buffer, Modulator *mod) {
        _buffer = buffer;
        _mod = mod;
    }

    void SetGateOpen(bool open) {
      if (open && !_is_gate_open) {
        _volume = 1.f;
        _last_playhead = 0;
        _Activate(0);
        _is_playing = true;
      }
      _is_gate_open = open;
    }

    bool IsPlaying() {
      return _is_playing;
    }

    void SetRelease(const float value) {
      if (value <= 0.002f) {
        _mode = Mode::one_shot;
      }
      else if (value >= 0.998f) {
        _mode = Mode::loop;
      }
      else {
        _mode = Mode::release;
        auto v = fmap(value, 0, 1, Mapping::EXP);
        _release_kof = (1.f - v) * 0.000139f * (1 - 0.9 * value);
      }
    }

    void ToggleDirection() {
      //_direction = -_direction;
    }

    void SetSpeed(const float value) {
        _delta = value;
    }

    void SetRandAmount(const float value) {
      _rand_amount = value;
    }

    void SetLoop(const float loop_start, const float loop_length) {
      _loop_start = static_cast<size_t>(loop_start * _buffer->Length());
      
      // Quantize loop length to the window slope. Minimum is 2 slopes = 1 window.
      // This gives 4ms precision (win_slope = 192), which is 250 points on the turn.
      auto new_length = static_cast<size_t>(loop_length * _buffer->Length());
      auto norm_length = (new_length / win_slope) * win_slope;
      if (new_length - norm_length > win_slope / 2) norm_length += win_slope;
      _loop_length = std::max(2 * win_slope, norm_length);
    }
  
    void Process(float& out0, float& out1) {
      out0 = 0.f;
      out1 = 0.f;

      if (!_is_playing) return;
      
      for (auto& w: _wins) {
        if (w.IsHalf()) _Activate(w.PlayHead());
      }

      if (!_is_gate_open) {
        if (_mode == Mode::release) _volume -= _release_kof * _volume;
        if (_volume <= .01f) {
          _is_playing = false;
          return;
        }
      }

      auto w_out0 = 0.f;
      auto w_out1 = 0.f;
      for (auto& w: _wins) {
          if (!w.IsActive()) continue;
          w_out0 = 0.f;
          w_out1 = 0.f;
          w.Process(_buffer, w_out0, w_out1);
          out0 += w_out0 * _volume;
          out1 += w_out1 * _volume;
      }
    }

private:
    void _Activate(float play_head) {
      if (_direction > 0 && play_head < _last_playhead || _direction < 0 && play_head > _last_playhead) {
        if (_mode == Mode::one_shot) _is_playing = false;
        if (_rand_amount > 0.02) {
          auto length = _buffer->Length();
          _loop_start_offset = length * 0.5 * _rand_amount * _mod->Value();
          if (_loop_start_offset < 0) _loop_start_offset += _loop_start + length;
        }
      }
      _last_playhead = play_head;
      for (auto& w: _wins) {
          if (!w.IsActive()) {
              w.Activate(play_head, _delta * _direction, _loop_start + _loop_start_offset, _loop_length);
              break;
          }
      }
    }

    enum class Mode {
      one_shot,
      loop,
      release
    };

    static constexpr size_t kMinLoopLength = 2 * win_slope;

    Buffer* _buffer;
    Modulator* _mod;
    std::array<Window<win_slope>, 2> _wins;

    float _delta;
    float _direction;
    float _rand_amount;
    float _volume;
    float _release_kof;
    float _last_playhead;
    size_t _loop_start;
    int32_t _loop_start_offset;
    size_t _loop_length;
    Mode _mode;
    bool _is_playing;
    bool _is_gate_open;
    
};
};
