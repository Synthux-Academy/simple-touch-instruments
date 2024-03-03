#pragma once;
#include <array>

namespace synthux {

class Detector {
public:
  Detector(): 
    _db_threshold  { -40.f },
    _average       { 0 },
    _write_head    { 0 },
    _read_head     { 0 },
    _is_armed      { false },
    _is_open       { false },
    _will_close    { false } {
      _buffer_0.fill(0);
      _buffer_1.fill(0);
    }

  void SetArmed(bool armed) {
    if (armed && !_is_armed) {
      _write_head = 0;
      _read_head = 0;
      _average = 0;
      _will_close = false;
    }
    _is_armed = armed;
  }

  bool IsArmed() {
    return _is_armed;
  }

  void SetTreshold(float value) {
    if (value <= 0) _db_threshold = -90.f;
    else _db_threshold = 20.f * daisysp::fastlog10f(value);
  }

  bool IsOpen() {
    return _is_open && !_will_close;
  }

  void Process(const float in0, const float in1, float& out0, float& out1) {
    if (_is_open) {
      // Read the buffer
      out0 = _buffer_0[_read_head];
      out1 = _buffer_1[_read_head];

      if (++_read_head == kWindow) {
        _read_head = 0;
        
        // If it's the last cycle, close the gate.
        if (_will_close) {
          _is_open = false;
          _will_close = false;
        }
        else if (!_is_armed) {
          // After recording is set to off, we do one more cycle, so the 
          // downstream Buffer has a time to fade out.
          _will_close = true;
        }
      }
    }
    else {
      out0 = 0;
      out1 = 0;
    }

    if (!_is_armed && !_will_close) {
      return;
    }
    
    // Write buffer
    _buffer_0[_write_head] = in0;
    _buffer_1[_write_head] = in1;

    // Make sample positive
    auto abs_in = abs(in0);

    // Exponential Moving Average (EVA) aka IIR one-pole filter.
    // Because kKofRise is way higher than
    // kKofFall, the contribution of the samples above
    // average is higher than of those below.
    // So the detector has fast attack and slow release.
    auto kof = (abs_in > _average) ? kKofRise : kKofFall;
    _average = abs_in + kof * (_average - abs_in);
    
    if (++_write_head == kWindow) {
      // Convert average to dB
      auto db_average = 20.f * daisysp::fastlog10f(_average);

      // If average exceedes the treshold open the gate
      if (db_average >= _db_threshold) {
        _read_head = 0;
        _is_open = true;
      }
      _average = 0;
      _write_head = 0;
    }
  }

private:
  static constexpr size_t kWindow = 480; //10ms @48k
  static constexpr float kKofRise = 0.1;
  static constexpr float kKofFall = 0.99;

  std::array<float, kWindow> _buffer_0;
  std::array<float, kWindow> _buffer_1;
  
  float   _average;
  float   _db_threshold;
  size_t  _write_head;
  size_t  _read_head;
  bool    _is_armed;
  bool    _is_open;
  bool    _will_close;
};
};
