// SYNTHUX ACADEMY /////////////////////////////////////////
// SIMPLE BASS /////////////////////////////////////////////
#pragma once
#include <array>
#include <random>
#include <functional>

#include "DaisyDSP.h"

#include "clk.h"
#include "trigger.h"
#include "cpattern.h"
#include "arp.h"
#include "driver.h"
#include "scale.h"
#include "vox.h"
#include "flt.h"
#include "xfade.h"

namespace synthux {

class Bass {
public:
  struct VoxParams {
    float osc1_shape;
    float osc1_pitch;
    float osc2_pitch;
    float osc2_amnt;
    float env;
    uint8_t osc2_mode_index;
  };

  struct FilterParams {
    float freq;
    float reso;
    float env_amount;
  };

  Bass():
  _dice               { std::uniform_int_distribution<uint8_t>(0, 100) },
  _tempo              { .45f },
  _env                { 0.f },
  _human_env_kof      { 0.f },
  _human_env_chance   { 0.f },
  _human_note_chance  { 0.f },
  _scale_index        { 0 },
  _is_arp_on          { false },
  _is_latched         { false }
  {
    _reverb_in.fill(0);
    _reverb_out.fill(0);
    _bus.fill(0);
    _hold.fill(false);
  }

  ~Bass() {}
  
  void Init(const float sample_rate, const float buffer_size) {
    using namespace std::placeholders;

    _clock.Init(sample_rate, buffer_size);

    auto on_clock = std::bind(&Bass::_on_clock_tick, this);
    _clock.SetOnTick(on_clock);

    auto on_arp_note_on = std::bind(&Bass::_on_arp_note_on, this, _1, _2);
    auto on_arp_note_off = std::bind(&Bass::_on_arp_note_off, this, _1);
    _arp.SetOnNoteOn(on_arp_note_on);
    _arp.SetOnNoteOff(on_arp_note_off);
    _arp.SetDirection(ArpDirection::fwd);
    _arp.SetRandChance(0);
    _arp.SetAsPlayed(true);
  
    auto on_driver_note_on = std::bind(&Bass::_on_driver_note_on, this, _1, _2, _3);
    auto on_driver_note_off = std::bind(&Bass::_on_driver_note_off, this, _1);
    _driver.SetOnNoteOn(on_driver_note_on);
    _driver.SetOnNoteOff(on_driver_note_off);

    for (auto& v: _voices) {
      v.Init(sample_rate);
    }

    _filter.Init(sample_rate);

    _reverb.Init(sample_rate);
    _reverb.SetFeedback(0.8);
    _reverb.SetLpFreq(10000.f);
  }

  void SetTempo(const float tempo) { 
    _clock.SetTempo(tempo); 
  }

  void SpeedUp() {
    _tempo = std::min(_tempo + .05f, 1.f);
    SetTempo(_tempo);
  }

  void SlowDown() {
    _tempo = std::max(_tempo - .05f, 0.05f);
    SetTempo(_tempo);
  }
  
  void ProcessClockIn(const bool state) { 
    _clock.Process(state); 
  }

  void SetArpOn(const bool value) {
    if (value != _is_arp_on) Reset();
    _is_arp_on = value;
    auto env_mode = value ? Envelope::Mode::AR : Envelope::Mode::ASR;
    for (auto& v: _voices) v.SetEnvelopeMode(env_mode);
    _filter.SetEnvelopeMode(env_mode);
  }

  bool IsLatched() {
    return _is_latched;
  }

  void SetLatch(const bool latch) {
    if (_is_latched && !latch) {
      for (auto i = 0; i < _hold.size(); i++) {
        if (_hold[i]) {
          _arp.NoteOff(i);
          _hold[i] = false;
        }
      }
      if (!_arp.HasNote()) Reset();
    }
    _is_latched = latch;
  }

  void ToggleMonoPoly() {
    if (_driver.IsMono()) _driver.SetPoly();
    else _driver.SetMono();
  }

  void NextScale() {
    _scale_index = std::min(static_cast<uint8_t>(_scale.ScalesCount() - 1), ++_scale_index);
    _scale.SetScaleIndex(_scale_index);
  }

  void PrevScale() {
    if (_scale_index == 0) return;
    _scale_index--;
    _scale.SetScaleIndex(_scale_index);
  }

  void NoteOn(const uint8_t note) {
    if (!_is_arp_on) {
      _driver.NoteOn(note);
      return;
    }

    if (_is_latched && _hold[note]) {
      _arp.NoteOff(note);
      _hold[note] = false;
    }
    else {
      _arp.NoteOn(note, 127);
      _hold[note] = true;
    }

    if (_arp.HasNote()) {
      if (!_clock.IsRunning()) _clock.Run();
    }
    else {
      Reset();
    }
  }

  void NoteOff(const uint8_t note) {
    if (!_is_arp_on) {
      _driver.NoteOff(note);
      _hold[note] = false;
      return;
    }
    if (!_is_latched) {
      _arp.NoteOff(note);
      _hold[note] = false;
    }
    if (!_arp.HasNote()) {
      Reset();
    }
  }

  void AllNotesOff() { 
    _arp.Clear();
    _driver.AllOff();
  }

  void Reset() {
    _clock.Stop();
    _trigger.Reset();
    _pattern.Reset();
    AllNotesOff();
  }

  void SetPattern(const float value) {
    _pattern.SetOnsets(value);
  }

  void SetHumanNoteChance(const float value) {
    _human_note_chance = std::clamp(static_cast<int>(value * 100), 0, 100);
  }

  void SetHumanEnvelopeChance(const float value) {
    _human_env_chance = std::clamp(static_cast<int>(value * 100), 0, 100);
    _human_env_kof = _human_env_chance * 0.0001f;
  }

  void SetVoxParams(const VoxParams& p) {
    auto osc1_mult = _scale.TransMult(p.osc1_pitch);
    
    auto osc2_mult = 1.f;
    Vox::Osc2Mode mode;
    switch (p.osc2_mode_index) {
      case 0: 
      mode = Vox::Osc2Mode::sound; 
      osc2_mult = _scale.TransMult(p.osc2_pitch);
      break;
      
      default: 
      mode = Vox::Osc2Mode::am; 
      osc2_mult = 99 * p.osc2_pitch * p.osc2_pitch;
      break;
    }
    
    _env = p.env;

    for (auto& v: _voices) {
      v.SetOsc1Shape(p.osc1_shape);
      v.SetOsc1Mult(osc1_mult);
      v.SetOsc2Mult(osc2_mult);
      v.SetOsc2Amount(p.osc2_amnt);
      v.SetOsc2Mode(mode);
    }
  }

  void SetFilterParams(const FilterParams& p) {
    _filter.SetFreq(p.freq);
    _filter.SetReso(p.reso);
    _filter.SetEnvelopeAmount(p.env_amount);
  }

  void SetReverbMix(const float value) {
    _xfade.SetStage(value);  
  }

  void Process(float **out, size_t size) {
    _clock.Tick();
    float output;
    for (size_t i = 0; i < size; i++) {
      output = 0;
      for (auto k = 0; k < kVoxCount; k++) {
        output += _voices[k].Process() * .5f;
      }
      _bus[0] = _bus[1] = _filter.Process(output);
      _xfade.Process(0, 0, _bus[0], _bus[1], _reverb_in[0], _reverb_in[1]);
      _reverb.Process(_reverb_in[0], _reverb_in[1], &(_reverb_out[0]), &(_reverb_out[1]));
      out[0][i] = (_bus[0] + _reverb_out[0]) * .75f;
      out[1][i] = (_bus[1] + _reverb_out[1]) * .75f;
    }
  }

private:
  void _on_clock_tick() { 
    if (_trigger.Tick() && _pattern.Tick()) {
      _arp.Trigger();
    }
  }

  void _on_arp_note_on(uint8_t num, uint8_t vel) { 
    _driver.NoteOn(num);
  }

  void _on_arp_note_off(uint8_t num) {
    _driver.NoteOff(num);
  }

  void _on_driver_note_on(uint8_t vox_idx, uint8_t num, bool retrigger) {
    auto h_env = _is_arp_on ? _humanized_envelope(_env, _pattern.Length()) : _env;
    auto freq = _scale.FreqAt(num, _is_arp_on ? _human_note_chance : 0);
    auto& v = _voices[vox_idx];
    _filter.SetEnvelope(h_env);
    _filter.Trigger(retrigger);
    v.SetEnvelope(h_env);
    v.NoteOn(freq, 1.f, retrigger);
  }

  void _on_driver_note_off(uint8_t vox_idx) {
    if (!_is_arp_on) {
      _voices[vox_idx].NoteOff();
      if (!_driver.HasNotes()) _filter.Release();
    }
  }

  float _humanized_envelope(float env, uint8_t length) {
    if (_human_env_chance <= 2) return env;
    auto human_env_chance_dice = _dice(_rand_engine);
    if (human_env_chance_dice < _human_env_chance) {
      auto delta = static_cast<float>(_dice(_rand_engine) - 50);
      if (delta > 0) {
        delta *= _human_env_kof * length;
      }
      else { 
        delta *= _human_env_kof;
      }
      return fclamp(env + delta, 0.f, 1.f); 
    }
  }

  static constexpr uint8_t kPPQN = 24;
  static constexpr uint8_t kNotesCount = 7;
  static constexpr uint8_t kVoxCount = 4;

  std::array<Vox, kVoxCount>  _voices;
  Driver<kVoxCount>           _driver;
  Scale                       _scale;
  Clock<kPPQN>                _clock;
  Trigger<kPPQN>              _trigger;
  CPattern                    _pattern;
  Arp<kNotesCount, 4>         _arp;
  Filter                      _filter;
  ReverbSc                    _reverb;
  XFade                       _xfade;

  std::default_random_engine _rand_engine;
  std::uniform_int_distribution<uint8_t> _dice;

  std::array<float, 2> _reverb_in;
  std::array<float, 2> _reverb_out;
  std::array<float, 2> _bus;
  
  float   _tempo;
  float   _env;
  float   _human_env_kof;
  uint8_t _human_env_chance;
  uint8_t _human_note_chance;
  uint8_t _scale_index;
  
  bool _is_arp_on;
  bool _is_latched;
  std::array<bool, kNotesCount> _hold;
};

};
