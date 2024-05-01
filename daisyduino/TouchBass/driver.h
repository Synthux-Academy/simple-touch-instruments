#pragma once;
#include "vox.h"
#include <array>

namespace synthux {

template<uint8_t max_vox_count>
class Driver {
public:
  Driver():
    _active_count { 0 },
    _vox_count    { max_vox_count } {
    Reset();
  }

void NoteOn(uint8_t num) {
  auto is_stealing = false;
  if (_vox_count > 1 && _active_count == _vox_count) {
    is_stealing = true;
    _release_at(0);
  }

  if (_vox_count == 1) {
      _notes[0] = num;
      _order[0] = num;
      _active_count = 1;
      _on_note_on(0, num, false);
      return;
  }
  
  for (auto i = 0; i < _vox_count; i++) {
    if (_notes[i] == kNone) {
      _notes[i] = num;
      _order[_active_count] = num;
      _active_count++;
      _on_note_on(i, num, is_stealing);
      break;
    }
  }
} 

void NoteOff(uint8_t num) {
  for (auto i = 0; i < _vox_count; i++) {
    if (_order[i] == num) {
      _release_at(i);
      break;
    }
  }
}

bool IsMono() {
  return _vox_count == 1;
}

void SetMono() {
  Reset();
  _vox_count = 1;
}

void SetPoly() {
  Reset();
  _vox_count = max_vox_count;
}

uint8_t ActiveCount() {
  return _active_count;
}

void Reset() {
  for (auto i = 0; i < _active_count; i++) _release_at(i);
  _notes.fill(kNone);
  _order.fill(kNone);
  _active_count = 0;
}

void SetOnNoteOn(void(*on_note_on)(uint8_t, uint8_t, bool)) {
  _on_note_on = on_note_on;
}

void SetOnNoteOff(void(*on_note_off)(uint8_t vox)) {
  _on_note_off = on_note_off;
}

private:
  void _release_at(uint8_t index) {
    auto note = _order[index];
    uint8_t i;
    for (i = 0; i < _vox_count; i++) {
      if (_notes[i] == note) {
        _active_count--;
        _on_note_off(i);
        _notes[i] = kNone;
        break;
      }
    }
    for (i = index; i < _vox_count - 1; i++) {
      _order[i] = _order[i+1];
    }
    _order[i] = kNone;
  }

  static constexpr uint8_t kNone = 0xff;

  void(*_on_note_on)(uint8_t vox, uint8_t num, bool steal);
  void(*_on_note_off)(uint8_t vox);

  std::array<uint8_t, max_vox_count> _notes;
  std::array<uint8_t, max_vox_count> _order;
  
  uint8_t _active_count;
  uint8_t _vox_count;
  
};
};
