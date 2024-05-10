#include "WSerial.h"
#pragma once;
#include <array>

namespace synthux {

template<uint8_t max_vox_count, class V>
class Driver {
public:
  Driver():
    _note_on_count { 0 },
    _is_stealing  { false },
    _vox_count    { max_vox_count } {
    Reset();
  }

void SetIsStealing(const bool value) {
  _is_stealing = value;
}

void NoteOn(uint8_t num) {
  if (_vox_count == 1) {
      _notes[0] = num;
      _order[0] = num;
      _note_on_count = 1;
      _on_note_on(0, num, false);
      return;
  }

  auto vox_idx = kNone;  
  for (auto i = 0; i < _vox_count; i++) {
    if (_is_stealing) {
      if (!(*_voices)[i].IsRunning()) {
        vox_idx = i;
        break;
      }
    }
    else if (_notes[i] == kNone) {
      vox_idx = i;
      break;
    }
  }

  auto retrigger = false;
  if (vox_idx == kNone) {
    retrigger = true;
    vox_idx = (_note_on_count > 0) ? _release_at(0) : 0;
  }

  _notes[vox_idx] = num;
  _order[_note_on_count] = num;
  _note_on_count++;
  _on_note_on(vox_idx, num, retrigger);
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
  return _note_on_count;
}

void Reset() {
  for (auto i = 0; i < _note_on_count; i++) _release_at(i);
  _notes.fill(kNone);
  _order.fill(kNone);
  _note_on_count = 0;
}

void SetOnNoteOn(void(*on_note_on)(uint8_t, uint8_t, bool)) {
  _on_note_on = on_note_on;
}

void SetOnNoteOff(void(*on_note_off)(uint8_t vox)) {
  _on_note_off = on_note_off;
}

void SetVoices(std::array<V, max_vox_count>* voices) {
  _voices = voices;
}

private:
  uint8_t _release_at(uint8_t index) {
    uint8_t vox_idx = 0;
    auto note = _order[index];
    uint8_t i;
    for (i = 0; i < _vox_count; i++) {
      if (_notes[i] == note) {
        _note_on_count--;
        _on_note_off(i);
        _notes[i] = kNone;
        vox_idx = i;
        break;
      }
    }
    for (i = index; i < _vox_count - 1; i++) {
      _order[i] = _order[i+1];
    }
    _order[i] = kNone;
    return vox_idx;
  }

  static constexpr uint8_t kNone = 0xff;

  void(*_on_note_on)(uint8_t vox, uint8_t num, bool steal);
  void(*_on_note_off)(uint8_t vox);

  std::array<V, max_vox_count>* _voices;
  std::array<uint8_t, max_vox_count> _notes;
  std::array<uint8_t, max_vox_count> _order;
  
  uint8_t _note_on_count;
  uint8_t _vox_count;
  bool _is_stealing;
  
};
};
