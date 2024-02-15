#pragma once

namespace synthux {

class Buffer {
  public:
    Buffer(): 
    _buffer_length      { 0 },
    _max_loop_length    { 0 },
    _rec_head           { 0 },
    _envelope_position  { 0 },
    _envelope_slope     { 0 },
    _envelope_slope_kof { 1 },
    _state              { State::idle },
    _is_full            { false }
    {}
  
    void Init(float **buf, size_t length, size_t envelope_slope = 192) {
      _buffer = buf;
      _buffer_length = length;
      _envelope_slope= envelope_slope;
      _envelope_slope_kof = 1.f / static_cast<float>(envelope_slope);
      // Reset buffer contents to zero
      memset(_buffer[0], 0, sizeof(float) * length);
      memset(_buffer[1], 0, sizeof(float) * length);
    }

    size_t Length() {
      return _max_loop_length;
    }

    void SetRecording(bool is_rec_on) {
      if (_state == State::idle) {
        if (is_rec_on) {
          _state = State::fadein;  
        }
      }
      else if (!is_rec_on) {
        _state = State::fadeout;
      }
    }

    bool IsRecording() {
      return _state != State::idle;
    }

    void Read(size_t frame, float& out0, float& out1) {
      frame %= _max_loop_length;
      out0 = _buffer[0][frame];
      out1 = _buffer[1][frame];
    }

    void Write(const float in0, const float in1) {
      switch (_state) {
        case State::idle: 
          return;

        case State::fadein:
          if (++_envelope_position == _envelope_slope) { 
            _state = State::sustain;
          };
          break;

        case State::sustain: 
          break;

        case State::fadeout:
          if (--_envelope_position == 0) {
            _state = State::idle;
          }
          break;
      }
      // Calculate fade in/out attenuation
      auto rec_attenuation = static_cast<float>(_envelope_position) * _envelope_slope_kof;
      auto inv_rec_attenuation = 1.f - rec_attenuation;

      //Write buffer
      _buffer[0][_rec_head] = in0 * rec_attenuation + _buffer[0][_rec_head] * inv_rec_attenuation;
      _buffer[1][_rec_head] = in1 * rec_attenuation + _buffer[1][_rec_head] * inv_rec_attenuation;
      
      //Advance rec head
      if (++_rec_head == _buffer_length) {
        _is_full = true;
        _rec_head = 0;
      }
      _max_loop_length = _is_full ? _buffer_length : _rec_head;
    }

  private:
    enum class State {
      idle,
      fadein,
      fadeout,
      sustain
    };
    
    float** _buffer;
    size_t  _buffer_length;
    size_t  _max_loop_length;
    size_t  _rec_head;
    size_t  _envelope_position;
    size_t  _envelope_slope;
    float   _envelope_slope_kof;
    State   _state;
    bool    _is_full;
};
};
