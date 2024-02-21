#pragma once;
#include <array>

namespace synthux {

class Detector {
public:
  Detector(): 
    _treshold     { 0.5 },
    _kof_rise     { 0.1 },
    _kof_fall     { 0.99 },
    _average      { 0 },
    _write_head   { 0 },
    _read_head    { 0 },
    _is_armed     { false },
    _is_open      { false } {
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
    _treshold = fmap(value, 0.f, 1.f, Mapping::EXP);
  }

  bool IsOpen() {
    return _is_open;
  }

  void Process(const float in0, const float in1, float& out0, float& out1) {
    if (_is_open) {
      out0 = _buffer_0[_read_head];
      out1 = _buffer_1[_read_head];
      if (++_read_head == kLength) {
        _read_head = 0;
        if (_will_close) {
          _is_open = false;
          _will_close = false;
        }
        else if (!_is_armed) {
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
    
    _buffer_0[_write_head] = in0;
    _buffer_1[_write_head] = in1;

    auto abs_in = abs(in0);
    if (abs_in > _average) {
      _average = _kof_rise * _average + (1.f - _kof_rise) * abs_in;
    } 
    else {
      _average = _kof_fall * _average + (1.f - _kof_fall) * abs_in;
    }
    
    if (++_write_head == kLength) {
      _check_average();
      _average = 0;
      _write_head = 0;
    }
  }

private:
  void _check_average() {
    if (10 * _average >= _treshold) {
      _read_head = 0;
      _is_open = true;
    }
  }

  static constexpr size_t kLength = 480; //10ms @48k

  std::array<float, kLength> _buffer_0;
  std::array<float, kLength> _buffer_1;
  
  float   _average;
  float   _kof_rise;
  float   _kof_fall;
  float   _treshold;
  size_t  _write_head;
  size_t  _read_head;
  bool    _is_armed;
  bool    _is_open;
  bool    _will_close;
};
};
