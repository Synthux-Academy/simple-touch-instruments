// SYNTHUX ACADEMY /////////////////////////////////////////
// ARPEGGIATED STRING //////////////////////////////////////

#include "simple-daisy-touch.h"
#include "clk.h"
#include "aknob.h"
#include "vox.h"
#include "arp.h"
#include "scale.h"
#include "cpattern.h"
#include "trigger.h"
#include "onoffon.h"
#include "mvalue.h"
#include "xfade.h"

using namespace synthux;

////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS //////////////////////////

static const uint8_t kAnalogResolution = 7; //7bits => 0..127
static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;
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

static AKnob<kAnalogResolution> bright_knob(A(S30));
static AKnob<kAnalogResolution> transp_knob(A(S31));
static AKnob<kAnalogResolution> struct_knob(A(S32));
static AKnob<kAnalogResolution> pattern_knob(A(S33));
static AKnob<kAnalogResolution> human_notes_knob(A(S34));
static AKnob<kAnalogResolution> human_verb_knob(A(S35));

static AKnob<kAnalogResolution> vol_drive_fader(A(S36));
static AKnob<kAnalogResolution> damp_fader(A(S37));

static MValue human_string_value;
static MValue verb_value;
static MValue pattern_value;
static MValue shift_value;

static OnOffOn arp_mode_switch(D(S07), D(S08));

// Uncomment this if you're 
// using external sync
// #define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clk_pin = D(S31);
#endif

////////////////////////////////////////////////////////////
//////////////////////// MODULES ///////////////////////////

static Scale scale;
static simpletouch::Touch touch;
static Clock<kPPQN> clck;
static Trigger<kPPQN> trig;
static CPattern pattern;
static Arp<kNotesCount, 4> arp;
static Vox vox;
static Overdrive drv;
static ReverbSc verb;
static XFade xfade;

////////////////////////////////////////////////////////////
////////////////////////// STATE ///////////////////////////

std::array<bool, kNotesCount> hold;
auto is_to_touched = false;
auto is_ch_touched = false;
auto latch = false;
auto tempo = .45f;
auto arp_on = false;
auto scale_index = 0;
auto volume = 1.f;

////////////////////////////////////////////////////////////
////////////////////// HUMANIZE ////////////////////////////

std::default_random_engine _rand_engine;
std::uniform_int_distribution<uint8_t> dice(0, 100);

uint8_t human_note_chance; //0...100
float humanized_note(uint8_t note) {
  auto freq = scale.FreqAt(note);
  if (human_note_chance <= 2) return freq;
  auto human_note_chance_dice = dice(_rand_engine);
  auto note_dice = dice(_rand_engine);
  auto octave_dice = dice(_rand_engine);
  if (human_note_chance < 33) {
    if (human_note_chance_dice < human_note_chance) {
      return (octave_dice < 50) ? freq * .5f : freq * 2.f;
    }
    else return freq;
  }
  else if (human_note_chance < 66) {
    if (human_note_chance_dice < human_note_chance) {
      if (note_dice < 50) freq = scale.Random();
      if (octave_dice < 25) return freq * .5f;
      else if (octave_dice > 75) return freq * 2.f; 
    }
    else return freq;
  }
  else {
    if (note_dice < human_note_chance) freq = scale.Random();
    if (octave_dice < 20) return freq * 2.f;
    else if (octave_dice > 80) return freq * .5f;
    else return freq; //60% of freq passes through unchanged
  }
}

float brightness;
float structure;
float damping;
uint8_t human_string_chance;
void humanize_string() {
  if (human_string_chance > 2) {
    auto chance_dice = dice(_rand_engine);
    if (chance_dice < human_string_chance) {
      auto bright_dice = dice(_rand_engine);
      auto structure_dice = dice(_rand_engine);
      auto damping_dice = dice(_rand_engine);

      auto bright_delta = bright_dice * 0.002f;
      brightness = std::min(brightness + bright_delta, 1.f);

      auto struct_delta = structure_dice * 0.002f;
      structure = std::min(structure + struct_delta, 1.f);

      auto damping_delta = damping_dice * 0.004f;
      damping = std::min(damping + damping_delta, 1.f);
    }
  }
  vox.SetBrightness(brightness);
  vox.SetStructure(structure);
  vox.SetDamping(damping);
}

////////////////////////////////////////////////////////////
///////////////////// CALLBACKS ////////////////////////////
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

  // Notes
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount - 1) return;
  auto note_num = pad - kFirstNotePad;
  if (!arp_on) {
    humanize_string();
    vox.NoteOn(scale.FreqAt(note_num), 1.f);
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
  pattern.Reset();
  arp.Clear();
}

void OnArpNoteOn(uint8_t num, uint8_t vel) {
  auto freq = arp_on ? humanized_note(num) : scale.FreqAt(num);
  humanize_string();
  vox.NoteOn(freq, 1.f);
}
void OnArpNoteOff(uint8_t num) {}

void OnClockTick() { 
  if (trig.Tick() && pattern.Tick()) arp.Trigger();
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float verb_in[2];
float verb_out[2];
float bus[2];
void AudioCallback(float **in, float **out, size_t size) {
  clck.Tick();
  for (size_t i = 0; i < size; i++) {
    bus[0] = bus[1] = drv.Process(vox.Process()) * volume;
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
  
  vox.Init(sample_rate);

  drv.Init();

  arp_mode_switch.Init();
  
  verb.Init(sample_rate);
  verb.SetFeedback(0.8);
  verb.SetLpFreq(10000.f);

  pattern_value.Init(1.f);
  shift_value.Init(0.f);

  pinMode(LED_BUILTIN, OUTPUT);

  analogReadResolution(kAnalogResolution);

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
  arp_on = arp_mode_value > 0;
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

  vox.SetMult(scale.TransMult(transp_knob.Process()));
  
  damping = damp_fader.Process();
  structure = struct_knob.Process();
  brightness = bright_knob.Process();
  
  human_note_chance = fmap(human_notes_knob.Process(), 0, 100);

  auto human_verb_knob_value = human_verb_knob.Process();
  verb_value.SetActive(is_to_touched, human_verb_knob_value);
  human_string_value.SetActive(!is_to_touched, human_verb_knob_value);

  auto pattern_shift_value = pattern_knob.Process();
  pattern_value.SetActive(!is_to_touched, pattern_shift_value);
  shift_value.SetActive(is_to_touched, pattern_shift_value);

  if (is_to_touched) {
    verb_value.Process(human_verb_knob_value);
    shift_value.Process(pattern_shift_value);
  }
  else {
    human_string_value.Process(human_verb_knob_value);
    pattern_value.Process(pattern_shift_value);
  }

  human_string_chance = fmap(human_string_value.Value(), 0, 100);
  
  pattern.SetOnsets(pattern_value.Value());
  pattern.SetShift(shift_value.Value());

  xfade.SetStage(verb_value.Value());

  auto drive = vol_drive_fader.Process();
  drv.SetDrive(0.2f  + drive * .4f);
  volume = 1.f - drive * 0.6;
  volume *= volume;

  delay(4);
}