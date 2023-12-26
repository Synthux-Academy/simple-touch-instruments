////////////////////////////////////////////////////////////
// STEREO AUDIO BUFFER /////////////////////////////////////

#pragma once

namespace synthux {

class Buffer {
  public:
    void Init(float **buf, size_t length, size_t env_slope = 192) {
      _buffer = buf;
      _buffer_length = length;
      _env_slope = env_slope;
      // Reset buffer contents to zero
      memset(_buffer[0], 0, sizeof(float) * length);
      memset(_buffer[1], 0, sizeof(float) * length);
    }

    size_t Length() {
      return _max_loop_length;
    }

    void SetRecordingLevel(float level) {
      _level = level;
    }

    void SetRecording(bool is_rec_on) {
        //Initialize recording head position on start
        if (_rec_env_pos_inc <= 0 && is_rec_on) {
            _rec_head = 0; 
        }
        // When record switch changes state it effectively
        // sets ramp to rising/falling, providing a
        // fade in/out in the beginning and at the end of 
        // the recorded region.
        _rec_env_pos_inc = is_rec_on ? 1 : -1;
    }
  
    bool IsRecording() {
      return _rec_env_pos > 0;
    }

    void Read(size_t frame, float& out0, float& out1) {
      frame %= _max_loop_length;
      out0 = _buffer[0][frame];
      out1 = _buffer[1][frame];
    }

    void Write(float& in0, float& in1) {
      // Calculate iterator position on the record level ramp.
      if ((_rec_env_pos_inc > 0 && _rec_env_pos < _env_slope)
       || (_rec_env_pos_inc < 0 && _rec_env_pos > 0)) {
          _rec_env_pos += _rec_env_pos_inc;
      }
      // If we're in the middle of the ramp - record to the buffer.
      if (IsRecording()) {
        // Calculate fade in/out
        float rec_attenuation = (static_cast<float>(_rec_env_pos - 1) / static_cast<float>(_env_slope - 1)) * _level;
        _buffer[0][_rec_head] = in0 * rec_attenuation + _buffer[0][_rec_head] * (1.f - rec_attenuation);
        _buffer[1][_rec_head] = in1 * rec_attenuation + _buffer[1][_rec_head] * (1.f - rec_attenuation);
        if (++_rec_head == _buffer_length) {
          _is_full = true;
          _rec_head = 0;
        }
        else {
          _max_loop_length = _is_full ? _buffer_length : _rec_head;
        }
      }
    }

  private:
    float** _buffer;

    float   _level              { 1.f };    
    size_t  _buffer_length      { 0 };
    size_t  _max_loop_length    { 0 };
    size_t  _rec_head           { 0 };
    size_t  _env_slope          { 192 };
    size_t  _rec_env_pos        { 0 };
    int32_t _rec_env_pos_inc    { 0 };
    bool _is_full               { false };
};
};
