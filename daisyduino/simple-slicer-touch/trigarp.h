////////////////////////////////////////////////////////////
// ARPEGGIATED TRIGGER /////////////////////////////////////

#pragma once

namespace synthux {

  enum class ArpDirection {
    fwd,
    rev
  };

  template<uint8_t trigger_count>
  class TrigArp {
  public:
    TrigArp():
      _on_trigger       { nullptr },
      _direction        { ArpDirection::fwd },
      _played_idx       { 0 },
      _as_played        { false },
      _bottom_idx       { 0 },
      _current_idx      { 0 },
      _size             { 0 } 
      {
        Reset();
      }

    void SetTrigger(uint8_t num, uint8_t vel) {
      //Currently we tolerate duplicates,
      //but if you want to ensure each note
      //is unique, uncomment:
      // NoteOff(num);

      //Look for a free slot.
      uint8_t slot = 1;
      while (slot < trigger_count + 1) {
        if (_triggers[slot].num == kEmpty) break;
        slot++;
      }
      // If slot is beyound the buffer, i.e. there's no free slots,
      // take the least recent note and replace it with the new one.
      if (slot == trigger_count + 1) {
        slot = _input_order[0];
        _RemoveNote(slot);
      }

      // Search for the index of the first note that
      // is higher than the one being inserted.
      auto idx = _bottom_idx;
      while (_triggers[idx].num < num) idx = _triggers[idx].next;

      // If the note that is higher than inserted
      // was the bottom one, the inserted becomes the bottom.
      if (idx == _bottom_idx) _bottom_idx = slot;

      // Insert the note, i.e. asign attributes and
      // link next / prev.
      _triggers[slot].num = num;
      _triggers[slot].vel = vel;
      _triggers[slot].next = idx;
      _triggers[slot].prev = _triggers[idx].prev;

      // Link previous/next notes
      _triggers[_triggers[idx].prev].next = slot;
      _triggers[idx].prev = slot;

      // Insert the slot index to the end of the
      // input order array.
      _input_order[_size - 1] = slot;

      _size ++;
    }

    void RemoveTrigger(uint8_t num) {
      // Serach for an index of the note that supposed to be off.
      // Here we're taking advantage of the fact that the 
      // container of the notes list is plain array so we can 
      // just for-loop through it.
      uint8_t idx = 0;
      for (uint8_t i = 0; i < trigger_count + 1; i++) {
        if (_triggers[i].num == num) {
            idx = i;
            break;
        }
      }
      if (idx != 0) _RemoveNote(idx);
    }

    // Register note on callback
    void SetOnTrigger(void(*on_trigger)(uint8_t num, uint8_t vel)) {
      _on_trigger = on_trigger;
    }

    void SetDirection(ArpDirection direction) {
      _direction = direction;
    }

    void SetAsPlayed(bool value) {
      _as_played = value;
    }

    void Tick() {
      // If only a sentinel note is there, i.e. no notes played, do nothing.
      if (_size <= 1) return;

      // Take next or previous note depending on direction
      uint8_t trigger_idx;
      switch (_direction) {
        case ArpDirection::fwd: trigger_idx = _NextNoteIdx(); break;
        case ArpDirection::rev: trigger_idx = _PrevNoteIdx(); break;
      }

      //Remeber the current note
      _current_idx = trigger_idx;

      //Trigger the note  
      _on_trigger(_triggers[trigger_idx].num, _triggers[trigger_idx].vel);
    }

    void Reset() {
      memset(_input_order, 0, sizeof(uint8_t) * trigger_count);
      _triggers[0].num = kSentinel;
      _triggers[0].next = 0;
      _triggers[0].prev = 0;
      _size = 1;
      for (uint8_t i = _size; i < trigger_count + 1; i++) {
          _triggers[i].num = kEmpty;
          _triggers[i].next = kUnlinked;
          _triggers[i].prev = kUnlinked;
      }
      _played_idx = 0;
      _bottom_idx = 0;
      _current_idx = 0; 
    }

  private:
    void _RemoveNote(uint8_t idx) {
      if (idx == _current_idx) _current_idx = _PrevNoteIdx();

      // Link next/previous notes to each other,
      // excluding the removed note from the chain.
      _triggers[_triggers[idx].prev].next = _triggers[idx].next;
      _triggers[_triggers[idx].next].prev = _triggers[idx].prev;
      if (idx == _bottom_idx) _bottom_idx = _triggers[idx].next;

      // "Remove" note, i.e. mark empty
      _triggers[idx].num = kEmpty;
      _triggers[idx].next = kUnlinked;
      _triggers[idx].prev = kUnlinked;

      // Remove the note from the input_order and 
      // rearrange the latter.
      for (uint8_t i = 0; i < _size - 1; i++) {
          if (_input_order[i] == idx) {
              while (i < _size - 1) {
                  _input_order[i] = _input_order[i + 1];
                  i++;
              }
          }
      }
      
      _size--;
      if (_size <= 1) _played_idx = 0;
    }

    uint8_t _NextNoteIdx() {
      if (_as_played) {
        _played_idx ++;
        if (_played_idx >= _size - 1) _played_idx = 0;
        return _input_order[_played_idx];
      }

      auto trigger_idx = _triggers[_current_idx].next;
      //Jump over sentinel note
      return (trigger_idx == 0) ? _triggers[trigger_idx].next : trigger_idx; 
    }

    uint8_t _PrevNoteIdx() {
      if (_as_played) {
        _played_idx = _played_idx == 0 ? _size - 2 : _played_idx - 1;
        return _input_order[_played_idx];
      }

      auto trigger_idx = _triggers[_current_idx].prev; 
      //Jump over sentinel note
      return (trigger_idx == 0) ? _triggers[trigger_idx].prev : trigger_idx;
    }

    struct Trigger {
      uint8_t num;
      uint8_t vel;
      uint8_t next;
      uint8_t prev;
    };

    static const uint8_t kSentinel = 0xff;
    static const uint8_t kEmpty    = 0xfe;
    static const uint8_t kUnlinked = 0xfd;

    void(*_on_trigger)(uint8_t num, uint8_t vel);

    Trigger _triggers[trigger_count + 1];
    uint8_t _input_order[trigger_count];

    ArpDirection _direction = ArpDirection::fwd;
    uint8_t _played_idx;
    bool _as_played;
    
    uint8_t _bottom_idx;
    uint8_t _current_idx;
    uint8_t _size;
  };
  
};
