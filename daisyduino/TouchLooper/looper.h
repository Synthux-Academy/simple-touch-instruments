#include "WSerial.h"
#pragma once
#include "buffer.h"
#include <array>
#include "DaisyDSP.h"
#include "hann.h"

namespace synthux {

enum class LooperSpeedMode {
      increment,
      shift
    };

enum class LooperPlayMode {
      one_shot,
      loop,
      release
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
    _loop_length        { 0 },
    _norm_length        { 1.f },
    _is_playing         { false },
    _is_gate_open       { false },
    _direction          { Direction::none },
    _is_retriggering    { false },
    _mode               { LooperPlayMode::loop },
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
          _is_playing = true;
        }
        else if (_mode != LooperPlayMode::loop) {
          _is_retriggering = true;
        }
        _volume = 1.f;
      }
      _is_gate_open = open;
    }

    bool IsPlaying() {
      return _is_playing;
    }

    void Stop() {
      _is_playing = false;
      for (auto& w: _wins) w.Deactivate();
    }

    void SetRelease(const float value) {
      if (value <= 0.002f) {
        _mode = LooperPlayMode::one_shot;
      }
      else if (value >= 0.998f) {
        _mode = LooperPlayMode::loop;
      }
      else {
        _mode = LooperPlayMode::release;
        _release_kof = 1.f / (value * value * kMaxReleaseTime * _sample_rate);
      }
    }

    LooperPlayMode Mode() {
      return _mode;
    }

    LooperSpeedMode SpeedMode() {
      return _speed_mode;
    }

    void SetSpeedMode(const LooperSpeedMode mode) {
      _speed_mode = mode;
    }

    void SetSpeed(const float value) {
       // Calculate delta for two speed change options
        _playhead_delta = kSlopeX2 * value - win_slope;
        _playhead_increment = 2.f * value;
        
        // Make a flat zone so it's easier to catch the normal speed
        if (value < 0.02) {
          _playhead_delta = -192.f;
        }
        else if (value > 0.45 && value < 0.55) {
          _playhead_delta = 0.f;
          _playhead_increment = 1.f;
        }
    }

    void SetReverse(const bool value) {
      _direction = value ? Direction::rev : Direction::fwd;
    }

    void SetStart(const float loop_start) {
      _loop_start = static_cast<size_t>(loop_start * (_buffer->Length() - 1));
    }

    void SetLength(const float norm_length) {
      _norm_length = norm_length;
      InvalidateLength();
    }

    void InvalidateLength() {
      _loop_length = max(static_cast<size_t>(_norm_length * _buffer->Length()), kSlopeX2);
    }

    void Process(float& out0, float& out1) {
      out0 = 0.f;
      out1 = 0.f;

      if (!_is_playing || _direction == Direction::none || _buffer->Length() == 0) return;
      
      for (auto& w: _wins) {
        if (!w.IsHalf()) continue;
        auto start = w.PlayHead();
        if (_mode == LooperPlayMode::one_shot && start >= _loop_length - win_slope) {
            Stop();
            continue;
        }

        if (_Activate(start)) {
          _is_retriggering = false;
          break;
        }
      }
      
      if (!_is_gate_open && _mode == LooperPlayMode::release) {
        if (_volume <= .01f) {
          Stop();
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
    bool _Activate(float playhead) { 
      _playhead = playhead;
      for (auto& w: _wins) {
          if (!w.IsActive()) {
              //Initially incrementing one sample at a time in either deirection
              auto increment = _direction == Direction::rev ? -1.f : 1.f;
              if (_speed_mode == LooperSpeedMode::shift) { //Variable shift mode
                // Upon start the first window starts with zero...
                if (playhead != 0) {
                  //... afterwards every window gets shifted by delta
                  playhead += _playhead_delta * increment;
                }
              }
              else { // Variable increment mode
                increment *= _playhead_increment;
              }
              w.Activate(playhead, increment, _loop_start, _loop_length);
              return true;
          }
      }
      return false;
    }

    enum class Direction {
      none,
      fwd,
      rev
    };

    static constexpr size_t kSlopeX2 = 2 * win_slope; 
    static constexpr float kSlopeKof = 1.f / static_cast<float>(win_slope);
    static constexpr float kMaxReleaseTime = 15.f; //seconds

    Buffer* _buffer;
    std::array<Window<win_slope>, 3> _wins;

    float _sample_rate;
    float _playhead;
    float _playhead_delta;
    float _playhead_increment;
    float _release_kof;
    float _volume;
    float _norm_length;
    size_t _loop_length;
    size_t _loop_start;
    Direction _direction;
    LooperPlayMode _mode;
    LooperSpeedMode _speed_mode;
    bool _is_playing;
    bool _is_gate_open;
    bool _is_retriggering;
    
};

template<size_t win_slope>
class Window {
public:
    Window():
    _playhead        { 0 },
    _playhead_delta  { 0 },
    _loop_start      { 0 },
    _iterator        { 0 },
    _is_active       { false }
    {}

    void Activate(float start, float delta, size_t loop_start, size_t loop_length) {
        _playhead = start;
        _loop_start = loop_start;
        _loop_length = loop_length;
        _playhead_delta = delta;
        _iterator = 0;
        _is_active = true;
    }

    bool IsActive() { return _is_active; }

    void Deactivate() {
      _is_active = false;
    }

    bool IsHalf() {  return _iterator == kHalf;  }

    float PlayHead() { return _playhead; }

    void Process(Buffer* buf, float& out0, float& out1) {
        // Do linear interpolation as playhead is float
        // Take integer part of the play head
        auto int_ph = static_cast<size_t>(_playhead);
        // Take fractional part
        auto frac_ph = _playhead - int_ph;
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
        
        _playhead += _playhead_delta;
        if (_playhead < 0) {
          _playhead += _loop_length;
        }
        else if (_playhead >= _loop_length) {
          _playhead -= _loop_length;
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

    float _playhead;
    float _playhead_delta;
    size_t _iterator;
    size_t _loop_start;
    size_t _loop_length;
    bool _is_active;
};

};
