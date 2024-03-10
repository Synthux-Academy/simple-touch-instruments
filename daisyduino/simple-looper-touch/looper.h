#pragma once
#include "buffer.h"
#include <array>
#include "DaisyDSP.h"
#include "hann.h"

namespace synthux {

enum class LooperSpeedMode {
      increment,
      delta
    };

template<size_t win_slope> class Window;

template<size_t win_slope = 192>
class Looper {
  public:
    Looper():
    _buffer             { nullptr },
    _playhead_delta     { 0.f },
    _playhead_increment { 0.f },
    _volume             { 1.f },
    _release_kof        { 0.f },
    _loop_start         { 0 },
    _win_per_loop       { 0 },
    _loop_length        { 1.f },
    _length_kof         { 1.f },
    _win_current        { 0 },
    _is_playing         { false },
    _is_gate_open       { false },
    _direction          { Direction::none },
    _is_retriggering    { false },
    _mode               { Mode::loop },
    _speed_mode         { LooperSpeedMode::increment }
    {}

    void Init(Buffer* buffer, float sample_rate) {
        _buffer = buffer;
        _sample_rate = sample_rate;
    }

    void SetGateOpen(bool open) {
      if (open && !_is_gate_open) {
        if (!_is_playing) {
          _Activate(0);
          _win_current = 0;
          _is_playing = true;
        }
        else if (_mode != Mode::loop) {
          _is_retriggering = true;
        }
        _volume = 1.f;
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
        _release_kof = 1.f / (value * value * kMaxReleaseTime * _sample_rate);
      }
    }

    LooperSpeedMode SpeedMode() {
      return _speed_mode;
    }

    void SetSpeedMode(const LooperSpeedMode mode) {
      _speed_mode = mode;
    }

    void SetSpeed(const float value) {
        _playhead_delta = kSlopeX2 * value - win_slope;
        _playhead_increment = 2.f * value;
        _direction = value > 0.5 ? Direction::fwd : Direction::rev;

        // Make a flat zone so it's easier to catch the normal speed
        if (value > 0.48 && value < 0.52) {
          _playhead_delta = 0.f;
          _length_kof = 1.f;
        }
        else {
          // Calculate correction coefficient for the loop length
          _length_kof = 1 / daisysp::fmax((2 * mapped_value), 0.01f);
        }
        InvalidateLength();
    }

    void SetStart(const float loop_start) {
      _loop_start = static_cast<size_t>(loop_start * (_buffer->Length() - 1));
    }

    void SetLength(const float loop_length) {
      _loop_length = loop_length;
      InvalidateLength();
    }
  
    void InvalidateLength() {
      // Quantize loop length to the half of the window. Minimum length is one window.
      // This gives 4ms precision (win_slope = 192 @48K).
      // Speed affects quantized loop length. The higher is the speed the shorter is the length.
      auto new_length = static_cast<size_t>(_loop_length * _buffer->Length() * _length_kof);
      _win_per_loop = std::max(static_cast<size_t>(new_length * kSlopeKof), static_cast<size_t>(2));
    }

    void Process(float& out0, float& out1) {
      out0 = 0.f;
      out1 = 0.f;

      if (!_is_playing || _direction == Direction::none || _buffer->Length() == 0) return;
      
      auto wrap = false;
      for (auto& w: _wins) {
        if (!w.IsHalf()) continue;
        if (_win_current >= _win_per_loop - 2) { //we're interested in the last but one window
          if (_mode == Mode::one_shot) {
            _Stop();
            continue;
          }
          wrap = true;
        }

        auto start = w.PlayHead();
        if (wrap || _is_retriggering) {
          start = _direction == Direction::rev ? _win_per_loop * win_slope - 1 : 0;
        }
        if (_Activate(start)) {
          _win_current = wrap || _is_retriggering ? 0 : _win_current + 1;
          _is_retriggering = false;
          break;
        }
      }

      
      if (!_is_gate_open && _mode == Mode::release) {
        if (_volume <= .01f) {
          _Stop();
          return;
        }
        else {
          daisysp::fonepole(_volume, 0, _release_kof);
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
    bool _Activate(float play_head) {
      for (auto& w: _wins) {
          if (!w.IsActive()) {
              auto increment = _direction == Direction::rev ? -1.f : 1.f;
              if (_speed_mode == LooperSpeedMode::delta) {
                play_head += _playhead_delta * increment;
              }
              else {
                increment *= _playhead_increment;
              }
              w.Activate(play_head, increment, _loop_start);
              return true;
          }
      }
      return false;
    }

    void _Stop() {
      _is_playing = false;
      for (auto& w: _wins) w.Deactivate();
    }

    enum class Mode {
      one_shot,
      loop,
      release
    };

    enum class Direction {
      none,
      fwd,
      rev
    };

    static constexpr float kSlopeX2 = 2.f * win_slope; 
    static constexpr float kSlopeKof = 1.f / static_cast<float>(win_slope);
    static constexpr float kMaxReleaseTime = 15.f; //seconds

    Buffer* _buffer;
    std::array<Window<win_slope>, 3> _wins;

    float _sample_rate;
    float _playhead_delta;
    float _playhead_increment;
    float _loop_length;
    float _length_kof;
    float _release_kof;
    float _volume;
    size_t _loop_start;
    size_t _win_per_loop;
    size_t _win_current;
    Mode _mode;
    Direction _direction;
    LooperSpeedMode _speed_mode;
    bool _is_playing;
    bool _is_zero_speed;
    bool _is_gate_open;
    bool _is_retriggering;
    
};

template<size_t win_slope>
class Window {
public:
    Window():
    _play_head    { 0 },
    _loop_start   { 0 },
    _playhead_delta        { 0 },
    _iterator     { 0 },
    _is_active    { false }
    {}

    void Activate(float start, float delta, size_t loop_start) {
        _play_head = start;
        _loop_start = loop_start;
        _playhead_delta = delta;
        _iterator = 0;
        _is_active = true;
    }

    bool IsActive() { return _is_active; }

    void Deactivate() {
      _is_active = false;
    }

    bool IsHalf() {  return _iterator == kHalf;  }

    float PlayHead() { return _play_head; }

    void Process(Buffer* buf, float& out0, float& out1) {
        // Do linear interpolation as playhead is float
        // Take integer part of the play head
        auto int_ph = static_cast<size_t>(_play_head);
        // Take fractional part
        auto frac_ph = _play_head - int_ph;
        // Take next integer inves
        auto next_ph = _playhead_delta > 0 ? int_ph + 1 : int_ph - 1;

        // Read the buffer
        auto a0 = 0.f;
        auto a1 = 0.f;
        auto b0 = 0.f;
        auto b1 = 0.f;
        buf->Read(int_ph + _loop_start, a0, a1);
        buf->Read(next_ph + _loop_start, b0, b1);

        // Apply window envelope
        auto att = _Attenuation();
        // Interpolate
        out0 = (a0 + frac_ph * (b0 - a0)) * att;
        out1 = (a1 + frac_ph * (b1 - a1)) * att;
        
        _play_head += _playhead_delta;
        if (_play_head < 0) {
          _play_head += buf->Length();
        }
        if (++_iterator == kSize) _is_active = false;
    }
  
private:
    float _Attenuation() {
      auto idx = (_iterator < kHalf) ? _iterator : kSize - _iterator - 1;
      return Hann<win_slope>::curve[idx];
    }

    static constexpr size_t kHalf { win_slope };
    static constexpr size_t kSize { 2 * win_slope };

    float _play_head;
    float _playhead_delta;
    size_t _iterator;
    size_t _loop_start;
    bool _is_active;
};

};
