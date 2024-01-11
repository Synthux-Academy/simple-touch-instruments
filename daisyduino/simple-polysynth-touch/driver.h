#pragma once;
#include "vox.h"
#include <array>

namespace synthux {

template<uint8_t vox_count>
class Driver {
public:
  Driver():
    _active_count { 0 } {
    _notes.fill(kNone);
    _order.fill(kNone);
  }

uint8_t onNoteOn(uint8_t num) {
  if (_active_count == vox_count) {
    _release_at(0);
  }
  auto vox_index = 0;
  for (auto i = 0; i < vox_count; i++) {
    if (_notes[i] == kNone) {
      _notes[i] = num;
      vox_index = i;
      break;
    }  
  }
  _order[_active_count] = num;
  _active_count++;
  return vox_index;
} 

void onNoteOff(uint8_t num) {
  for (auto i = 0; i < vox_count; i++) {
    if (_order[i] == num) {
      _release_at(i);
      break;
    }
  }
}

bool gateAt(uint8_t index) {
  return _notes[index] != kNone;
}

uint8_t noteAt(uint8_t index) {
  return _notes[index];
}

private:
  void _release_at(uint8_t index) {
    auto note = _order[index];
    uint8_t i;
    for (i = 0; i < vox_count; i++) {
      if (_notes[i] == note) {
        _notes[i] = kNone;
        break;
      }
    }
    for (i = index; i < vox_count - 1; i++) {
      _order[i] = _order[i+1];
    }
    _active_count--;
  }

  static constexpr uint8_t kNone = 0xff;

  std::array<uint8_t, vox_count> _notes;
  std::array<uint8_t, vox_count> _order;
  
  uint8_t _active_count;
};
};
