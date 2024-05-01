// SYNTHUX ACADEMY /////////////////////////////////////////
// ARPEGGIATED BASS //////////////////////////////////////

#include "simple-daisy-touch.h"
#include "clk.h"
#include "aknob.h"
#include "vox.h"
#include "arp.h"
#include "driver.h"
#include "scale.h"
#include "cpattern.h"
#include "trigger.h"
#include "onoffon.h"
#include "flt.h"
#include "mvalue.h"
#include "xfade.h"

using namespace synthux;

////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS //////////////////////////

static const uint8_t kPPQN = 24;
static const uint8_t kNotesCount = 7;
static constexpr uint8_t kVoxCount = 4;
static const uint16_t kFirstNotePad = 3;

////////////////////////////////////////////////////////////
///////////////////// KNOBS & SWITCHES /////////////////////
//
//    |-| (*)   (*)   (*)    (*) |-|
//    | | S31   S32   S33    S34 | |
//    |||                        |||
//    |_| (*)                (*) |_|
//    S36 S30                S35 S37
//
//      S10 o o S09    o S07
//                   o S08

static AKnob osc2_amount(A(S30));
static AKnob osc1_mult_knob(A(S31));
static AKnob osc2_mult_knob(A(S32));
static AKnob pattern_knob(A(S33));
static AKnob human_notes_knob(A(S34));
static AKnob human_verb_knob(A(S35));
static AKnob filter_fader(A(S36));
static AKnob env_fader(A(S37));

static OnOffOn osc2_mode_switch(D(S09), D(S10));
static OnOffOn arp_mode_switch(D(S07), D(S08));

static MValue flt_freq;
static MValue flt_reso;
static MValue flt_env_amount;
static MValue osc_1_freq;
static MValue osc_1_shape;
static MValue human_env_value;
static MValue verb_value;

// Unomment this if you're
// planning using external sync
// #define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clk_pin = D(S31);
#endif

////////////////////////////////////////////////////////////
//////////////////////// MODULES ///////////////////////////

static std::array<Vox, kVoxCount> vox;
static Driver<kVoxCount> driver;
static Scale scale;
static simpletouch::Touch touch;
static Clock<kPPQN> clck;
static Trigger<kPPQN> trig;
static CPattern ptn;
static Arp<kNotesCount, 4> arp;
static Filter flt;
static ReverbSc verb;
static XFade xfade;

////////////////////////////////////////////////////////////
////////////////////////// STATE ///////////////////////////

std::array<bool, kNotesCount> hold;
bool latch = false;
bool arp_on = false;
bool is_to_touched = false;
bool is_ch_touched = false;
auto scale_index = 0;
float tempo = .3f;
float env = 0;

////////////////////////////////////////////////////////////
////////////////////// HUMANIZE ////////////////////////////

std::default_random_engine _rand_engine;
std::uniform_int_distribution<uint8_t> dice(0, 100);

uint8_t human_note_chance;
float humanized_note(uint8_t note) {
  auto freq = scale.FreqAt(note);
  if (human_note_chance <= 2) return freq;
  auto human_note_chance_dice = dice(_rand_engine);
  auto note_dice = dice(_rand_engine);
  auto octave_dice = dice(_rand_engine);
  if (human_note_chance < 33) {
    if (human_note_chance_dice < human_note_chance * 3) {
      return (octave_dice < 50) ? freq * .5f : freq * 2.f;
    }
  }
  else if (human_note_chance < 66) {
    if (human_note_chance_dice < static_cast<uint8_t>(human_note_chance * 1.5f)) {
      if (note_dice >= 50) return scale.Random();
      else return (octave_dice < 50) ? freq * .5f : freq * 2.f; 
    }
  }
  else {
    if (human_note_chance_dice <= 100) {
      if (note_dice >= 50) return scale.Random();
      else if (octave_dice < 33) { return freq * 4.f; }
      else if (octave_dice > 66) { return freq * .25f; }
      else { return freq; }
    }
  }
}

float delta;
float human_env_kof;
uint8_t human_env_chance;
float humanized_envelope(float env, uint8_t length) {
  if (human_env_chance <= 2) return env;
  auto human_env_chance_dice = dice(_rand_engine);
  if (human_env_chance_dice < human_env_chance) {
    delta = static_cast<float>(dice(_rand_engine) - 50);
    if (delta > 0) {
      delta *= human_env_kof * length;
    }
    else { 
      delta *= human_env_kof;
    }
    return fclamp(env + delta, 0.f, 1.f); 
  }
}

////////////////////////////////////////////////////////////
///////////////////// TOUCH CALLBACKS //////////////////////
void OnPadTouch(uint16_t pad) {
 // Scale & Tempo
  if (pad == 0) {
    if (is_to_touched) tempo = max(tempo - .05f, 0.05f);
    else if (is_ch_touched) {
      scale_index = max(0, --scale_index);
      scale.SetScaleIndex(scale_index);
    }
    return;
  }
  if (pad == 2) {
    if (is_to_touched) tempo = min(tempo + .05f, 1.f);
    else if (is_ch_touched) { 
      scale_index = min(scale.ScalesCount() - 1, ++scale_index);
      scale.SetScaleIndex(scale_index);
    }
    return;
  }

  //Mono/poly
  if (pad == 11) {
    if (is_to_touched) {
      if (driver.IsMono()) driver.SetPoly();
      else driver.SetMono();
    }
  }

  //Notes
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
  auto note_num = pad - kFirstNotePad;
  if (!arp_on) {
    driver.NoteOn(note_num);
    return;
  }

  if (arp_on && latch && hold[note_num]) {
    arp.NoteOff(note_num);
    hold[note_num] = false;
  }
  else {
      arp.NoteOn(note_num, 127);
      hold[note_num] = true;
  }

  if (arp.HasNote()) {
    if (!clck.IsRunning()) clck.Run();
  }
  else {
    Reset();
  }
}
void OnPadRelease(uint16_t pad) {
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
  auto num = pad - kFirstNotePad;
  if (!arp_on) {
    driver.NoteOff(num);
    return;
  }
  if (!latch) { 
    arp.NoteOff(num);
    hold[num] = false;
  }
  if (!arp.HasNote()) {
    Reset();
  }
}
void Reset() {
  clck.Stop();
  trig.Reset();
  ptn.Reset();
  arp.Clear();
  driver.Reset();
}

////////////////////////////////////////////////////////////
////////////////////// ARP CALLBACKS ///////////////////////
void OnArpNoteOn(uint8_t num, uint8_t vel) { 
  driver.NoteOn(num);
}
void OnArpNoteOff(uint8_t num) {
  driver.NoteOff(num);
}

////////////////////////////////////////////////////////////
///////////////////// CLOCK CALLBACK ///////////////////////
void OnClockTick() { 
  if (trig.Tick() && ptn.Tick()) arp.Trigger();
}

////////////////////////////////////////////////////////////
//////////////////// DRIVER CALLBACKS ///////////////////////
void OnDriverNoteOn(uint8_t vox_idx, uint8_t num, bool is_stealing) {
  auto h_env = arp_on ? humanized_envelope(env, ptn.Length()) : env;
  auto freq = arp_on ? humanized_note(num) : scale.FreqAt(num);
  auto& v = vox[vox_idx];
  flt.SetEnvelope(h_env);
  flt.Trigger(driver.ActiveCount() == 1);
  v.SetEnvelope(h_env);
  v.NoteOn(freq, 1.f, driver.ActiveCount() != 1);
}

void OnDriverNoteOff(uint8_t vox_idx) {
  if (!arp_on) {
    vox[vox_idx].NoteOff();
    if (driver.ActiveCount() == 0) flt.Release();
  }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float output;
float verb_in[2];
float verb_out[2];
float bus[2];
void AudioCallback(float **in, float **out, size_t size) {
  clck.Tick();
  for (size_t i = 0; i < size; i++) {
    output = 0;
    for (auto k = 0; k < kVoxCount; k++) {
      output += vox[k].Process() * .6f;
    }
    bus[0] = bus[1] = flt.Process(output);
    xfade.Process(0, 0, bus[0], bus[1], verb_in[0], verb_in[1]);
    verb.Process(verb_in[0], verb_in[1], &(verb_out[0]), &(verb_out[1]));
    out[0][i] = (bus[0] + verb_out[0]) * .75f;
    out[1][i] = (bus[1] + verb_out[1]) * .75f;
  }
}

///////////////////////////////////////////////////////////////
////////////////////////// SETUP //////////////////////////////

void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  DAISY.SetAudioBlockSize(4);
  float sample_rate = DAISY.AudioSampleRate();
  float buffer_size = DAISY.AudioBlockSize();

  clck.Init(sample_rate, buffer_size);
  clck.SetOnTick(OnClockTick);

  #ifdef EXTERNAL_SYNC
  pinMode(clk_pin, INPUT);
  #endif

  touch.Init();
  touch.SetOnTouch(OnPadTouch);
  touch.SetOnRelease(OnPadRelease);

  arp.SetOnNoteOn(OnArpNoteOn);
  arp.SetOnNoteOff(OnArpNoteOff);
  
  driver.SetOnNoteOn(OnDriverNoteOn);
  driver.SetOnNoteOff(OnDriverNoteOff);

  flt_freq.Init(1.f);
  flt_reso.Init(.2f);
  flt_env_amount.Init(0.f);
  osc_1_freq.Init(.5f);
  osc_1_shape.Init(0.f);

  for (auto& v: vox) v.Init(sample_rate);

  flt.Init(sample_rate);

  osc2_mode_switch.Init();
  arp_mode_switch.Init();

  verb.Init(sample_rate);
  verb.SetFeedback(0.8);
  verb.SetLpFreq(10000.f);

  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

///////////////////////////////////////////////////////////////
////////////////////////// LOOP ///////////////////////////////

void loop() {
  clck.SetTempo(tempo);
  #ifdef EXTERNAL_SYNC
  clck.Process(!digitalRead(clk_pin));
  #endif

  digitalWrite(LED_BUILTIN, latch);

  touch.Process();

  is_to_touched = touch.IsTouched(10);
  is_ch_touched = touch.IsTouched(11);

  auto arp_mode_value = arp_mode_switch.Value();
  auto new_arp_on = arp_mode_value > 0;
  if (new_arp_on != arp_on) {
    driver.Reset();
  }
  arp_on = new_arp_on;
  auto new_latch = arp_mode_value > 1;
  if (latch && !new_latch) {
      //Drop all latched notes except ones being touched
      for (auto i = 0; i < hold.size(); i++) {
        if (hold[i] && !touch.IsTouched(i + kFirstNotePad))  {
          arp.NoteOff(i);
        }
      }
      //Reset latch memory
      hold.fill(false);
  }
  latch = new_latch;
  arp.SetDirection(ArpDirection::fwd);
  arp.SetRandChance(0);
  arp.SetAsPlayed(true);

  auto osc1_value = osc1_mult_knob.Process();
  auto osc1_mult = scale.TransMult(osc_1_freq.Value());
  osc_1_freq.SetActive(!is_to_touched, osc1_value);
  osc_1_shape.SetActive(is_to_touched, osc1_value);
  if (is_to_touched) osc_1_shape.Process(osc1_value);
  else osc_1_freq.Process(osc1_value);

  auto osc2_value = osc2_mult_knob.Process();
  auto osc2_mult = 1.f;
  auto mode = Vox::Osc2Mode::sound;
  auto mode_value = osc2_mode_switch.Value();
  switch (mode_value) {
    case 0: mode = Vox::Osc2Mode::sound; 
    osc2_mult = scale.TransMult(osc2_value);
    break;
    
    default: 
    mode = Vox::Osc2Mode::am; 
    osc2_mult = 99 * osc2_value * osc2_value;
    break;
  }

  auto osc2_amnt = fmap(osc2_amount.Process(), 0.f, 1.f, Mapping::EXP);
  auto env_mode = arp_on ? Envelope::Mode::AR : Envelope::Mode::ASR;
  env = env_fader.Process();
  
  for (auto& v: vox) {
    v.SetOsc1Shape(osc_1_shape.Value());
    v.SetOsc1Mult(osc1_mult);
    v.SetOsc2Mult(osc2_mult);
    v.SetOsc2Amount(osc2_amnt);
    v.SetOsc2Mode(mode);
    v.SetEnvelopeMode(env_mode);
  }

  human_note_chance = fmap(human_notes_knob.Process(), 0, 100);

  auto flt_value = filter_fader.Process();
  auto human_verb_knob_value = human_verb_knob.Process();
  verb_value.SetActive(is_to_touched, human_verb_knob_value);
  human_env_value.SetActive(!is_to_touched, human_verb_knob_value);
  flt_freq.SetActive(!is_to_touched, flt_value);
  flt_reso.SetActive(is_to_touched, flt_value);
  flt_env_amount.SetActive(is_ch_touched, flt_value);
  flt.SetEnvelopeMode(arp_on ? Envelope::Mode::AR : Envelope::Mode::ASR);
  if (is_to_touched) {
    verb_value.Process(human_verb_knob_value);
    flt_reso.Process(flt_value);
  }
  else if (is_ch_touched) {
    flt_env_amount.Process(flt_value);
  }
  else {
    human_env_value.Process(human_verb_knob_value);
    flt_freq.Process(flt_value);
  }
  
  flt.SetEnvelopeAmount(flt_env_amount.Value());

  human_env_chance = fmap(human_env_value.Value(), 0, 100);
  human_env_kof = human_env_chance * 0.0001f;
  xfade.SetStage(verb_value.Value());  

  flt.SetFreq(flt_freq.Value());
  flt.SetReso(flt_reso.Value());

  ptn.SetOnsets(pattern_knob.Process());

  delay(4);
}
