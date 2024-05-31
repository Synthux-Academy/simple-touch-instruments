#pragma once
#include <array>
#include <functional>

namespace synthux {

template<uint8_t max_vox_count>
class Driver {
public:
  Driver():
    _note_on_count  { 0 },
    _vox_count      { max_vox_count } {
    for (uint8_t i = 0; i < _vox_count; i++) {
      _notes[i] = kNone;
      _queue[i] = i;
      _active[i] = false;
    }
  }

void NoteOn(uint8_t note) {
  if (_vox_count == 1) {
      _notes[0] = note;
      _queue[0] = 0;
      _note_on_count = 1;
      _on_note_on(0, note, false);
      return;
  }
  auto vox = _vox_for_note(note);
  _notes[vox.index] = note;
  _active[vox.index] = true;
  _note_on_count++;
  _on_note_on(vox.index, note, vox.retrigger);
}

void NoteOff(uint8_t note) {
  _release_note(note);
}

bool IsMono() {
  return _vox_count == 1;
}

void SetMono() {
  AllOff();
  _vox_count = 1;
}

void SetPoly() {
  AllOff();
  _vox_count = max_vox_count;
}

bool IsNoteOn(const uint8_t note) {
  for (auto i = 0; i < _vox_count; i++) {
    if (_notes[i] == note && _active[i]) return true;
  }
  return false;
}

uint8_t HasNotes() {
  return _note_on_count > 0;
}

void AllOff() {
    for (auto i = 0; i < _vox_count; i++) {
        if (_active[i]) _release_note(_notes[i]);
    }
}

void SetOnNoteOn(std::function<void(uint8_t, uint8_t, bool)> on_note_on) {
  _on_note_on = on_note_on;
}

void SetOnNoteOff(std::function<void(uint8_t)> on_note_off) {
  _on_note_off = on_note_off;
}

private:
  struct Voice {
    uint8_t index;
    bool retrigger;
  };

  Voice _vox_for_note(uint8_t note) {
      Voice vox;
      uint8_t queue_idx = kNone;
      uint8_t i;
    
      // If one of the voice plays (or played) this note
      // this voice going to be chosen.
      for (i = 0; i < _vox_count; i++) {
          if (_notes[_queue[i]] != note) continue;
          vox.retrigger = false;
          queue_idx = i;
          break;
      }

      // If it's different note, look for the free one.
      if (queue_idx == kNone) {
        // Find free voice enumerating in the queue order
        for (i = 0; i < _vox_count; i++) {
          if (!_active[_queue[i]]) {
              queue_idx = i;
              vox.retrigger = true;
              break;
          }
        }
      }

      // If there's no voice found in previous
      // steps, take the first in the queue
      if (queue_idx == kNone) {
        queue_idx = 0;
        vox.retrigger = true;
      }

    // Take the voice from the queue.
    vox.index = _queue[queue_idx];

    // Move the taken voice to the end of the queue.
    for (i = queue_idx; i < _vox_count - 1; i++) {
      _queue[i] = _queue[i+1];
    }
    _queue[i] = vox.index;

    //
    return vox;
  }

  void _release_note(uint8_t note) {
    uint8_t i;
    for (i = 0; i < _vox_count; i++) {
      if (_notes[i] == note) {
        _active[i] = false;
        _note_on_count --;
        _on_note_off(i);
        break;
      }
    }
  }

  static constexpr uint8_t kNone = 0xff;

  std::function<void(uint8_t vox, uint8_t num, bool steal)> _on_note_on;
  std::function<void(uint8_t vox)> _on_note_off;

  std::array<bool, max_vox_count> _active;
  std::array<uint8_t, max_vox_count> _notes;
  std::array<uint8_t, max_vox_count> _queue;

  uint8_t _note_on_count;
  uint8_t _vox_count;

};
};
