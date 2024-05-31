#pragma once
#include <functional>
#include <random>

namespace synthux {

  enum class ArpDirection {
    fwd,
    rev
  };

  template<uint8_t note_count, uint8_t ppqn>
  class Arp {
  public:
    Arp():
      _on_note_on       { nullptr },
      _on_note_off      { nullptr },
      _direction        { ArpDirection::fwd },
      _dice             { std::uniform_int_distribution<uint8_t>(0, 100) },
      _rand_chance      { 0 },
      _played_idx       { 0 },
      _as_played        { false },
      _note_length      { ppqn / 12 },
      _max_note_length  { ppqn / 4 + 1 },
      _bottom_idx       { 0 },
      _current_idx      { 0 },
      _pulse_counter    { 0 },
      _size             { 0 }
      {
        _SetMaxNoteLength();
        Clear();
    }

    void NoteOn(uint8_t num, uint8_t vel) {
      //Currently we tolerate duplicates,
      //but if you want to ensure each note
      //is unique, uncomment:
      // NoteOff(num);

      //Look for a free slot.
      uint8_t slot = 1;
      while (slot < note_count + 1) {
        if (_notes[slot].num == kEmpty) break;
        slot++;
      }
      // If slot is beyound the buffer, i.e. there's no free slots,
      // take the least recent note and replace it with the new one.
      if (slot == note_count + 1) {
        slot = _input_order[0];
        _RemoveNote(slot);
      }

      // Search for the index of the first note that
      // is higher than the one being inserted.
      auto idx = _bottom_idx;
      while (_notes[idx].num < num) idx = _notes[idx].next;

      // If the note that is higher than inserted
      // was the bottom one, the inserted becomes the bottom.
      if (idx == _bottom_idx) _bottom_idx = slot;

      // Insert the note, i.e. asign attributes and
      // link next / prev.
      _notes[slot].num = num;
      _notes[slot].vel = vel;
      _notes[slot].next = idx;
      _notes[slot].prev = _notes[idx].prev;

      // Link previous/next notes
      _notes[_notes[idx].prev].next = slot;
      _notes[idx].prev = slot;

      // Insert the slot index to the end of the
      // input order array.
      _input_order[_size - 1] = slot;

      _size ++;
    }

    void NoteOff(uint8_t num) {
      // Serach for an index of the note that supposed to be off.
      // Here we're taking advantage of the fact that the 
      // container of the notes list is plain array so we can 
      // just for-loop through it.
      uint8_t idx = 0;
      for (uint8_t i = 0; i < note_count + 1; i++) {
        if (_notes[i].num == num) {
            idx = i;
            break;
        }
      }
      if (idx != 0) _RemoveNote(idx);
    }

    // Register note on callback
    void SetOnNoteOn(std::function<void(uint8_t num, uint8_t vel)> on_note_on) {
      _on_note_on = on_note_on;
    }

    // Register note off callback
    void SetOnNoteOff(std::function<void(uint8_t num)> on_note_off) {
      _on_note_off = on_note_off;
    }

    void SetDirection(ArpDirection direction) {
      _direction = direction;
    }

    void SetRandChance(float rand) {
      _rand_chance = rand;
    }

    void SetAsPlayed(bool value) {
      _as_played = value;
    }

    void SetNoteLength(float length) {
      _note_length = _max_note_length * length;
    }

    void Trigger() {
      // If only a sentinel note is there, i.e. no notes played, do nothing.
      if (!HasNote()) return;

      // "Release" last played note
      if (_note_length < ppqn / 4 && _pulse_counter == _note_length && _current_idx > 0) {
        _on_note_off(_notes[_current_idx].num);
      }
      
      // We trigger on every 1/16th
      if (++_pulse_counter < ppqn / 4) return;
      _pulse_counter = 0;

      // Take next or previous note depending on direction
      uint8_t note_idx;
      switch (_direction) {
        case ArpDirection::fwd: note_idx = _NextNoteIdx(); break;
        case ArpDirection::rev: note_idx = _PrevNoteIdx(); break;
      }

      // Randomize note
      if (_rand_chance > 5 && _rand_chance < 95) {
        if (_dice(_rand_engine) <= _rand_chance) {
          std::uniform_int_distribution<uint8_t> _note_distribution(0, _size - 2);
          note_idx = _input_order[_note_distribution(_rand_engine)];
        }
     }

      //Remeber the current note
      _current_idx = note_idx;

      //Trigger the note  
      _on_note_on(_notes[note_idx].num, _notes[note_idx].vel);
    }

    bool HasNote() {
      return _size > 1;
    }

    void Clear() {
      memset(_input_order, 0, sizeof(uint8_t) * note_count);
      _notes[0].num = kSentinel;
      _notes[0].next = 0;
      _notes[0].prev = 0;
      _size = 1;
      for (uint8_t i = _size; i < note_count + 1; i++) {
          _notes[i].num = kEmpty;
          _notes[i].next = kUnlinked;
          _notes[i].prev = kUnlinked;
      }
      _pulse_counter = ppqn / 4;
      _current_idx = 0;
    }

  private:
    void _RemoveNote(uint8_t idx) {
      _on_note_off(_notes[idx].num);

      if (idx == _current_idx) _current_idx = _PrevNoteIdx();

      // Link next/previous notes to each other,
      // excluding the removed note from the chain.
      _notes[_notes[idx].prev].next = _notes[idx].next;
      _notes[_notes[idx].next].prev = _notes[idx].prev;
      if (idx == _bottom_idx) _bottom_idx = _notes[idx].next;

      // "Remove" note, i.e. mark empty
      _notes[idx].num = kEmpty;
      _notes[idx].next = kUnlinked;
      _notes[idx].prev = kUnlinked;

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

      auto note_idx = _notes[_current_idx].next;
      //Jump over sentinel note
      return (note_idx == 0) ? _notes[note_idx].next : note_idx; 
    }

    uint8_t _PrevNoteIdx() {
      if (_as_played) {
        _played_idx = _played_idx == 0 ? _size - 2 : _played_idx - 1;
        return _input_order[_played_idx];
      }

      auto note_idx = _notes[_current_idx].prev; 
      //Jump over sentinel note
      return (note_idx == 0) ? _notes[note_idx].prev : note_idx;
    }

    void _SetMaxNoteLength() {
      _max_note_length = ppqn / 4;
    }

    struct Note {
      uint8_t num;
      uint8_t vel;
      uint8_t next;
      uint8_t prev;
    };

    static const uint8_t kSentinel = 0xff;
    static const uint8_t kEmpty    = 0xfe;
    static const uint8_t kUnlinked = 0xfd;

    std::function<void(uint8_t num, uint8_t vel)> _on_note_on;
    std::function<void(uint8_t num)> _on_note_off;

    Note _notes[note_count + 1];
    uint8_t _input_order[note_count];

    ArpDirection _direction = ArpDirection::fwd;
    std::default_random_engine _rand_engine;
    std::uniform_int_distribution<uint8_t> _dice;
    uint8_t _rand_chance;
    uint8_t _played_idx;
    bool _as_played;
    
    uint8_t _note_length;
    uint8_t _max_note_length;
    uint8_t _bottom_idx;
    uint8_t _current_idx;
    uint8_t _pulse_counter;
    uint8_t _size;
  };
};
