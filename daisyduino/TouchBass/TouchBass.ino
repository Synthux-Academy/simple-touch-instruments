// SYNTHUX ACADEMY /////////////////////////////////////////
// TOUCH BASS //////////////////////////////////////////////

#include "simple-daisy-touch.h"
#include "aknob.h"
#include "onoffon.h"
#include "mvalue.h"
#include "bass.h"

using namespace synthux;
using namespace simpletouch;

////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS //////////////////////////

static const uint8_t kNotesCount = 7;
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

// Unomment if you're planning using external sync
// #define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clk_pin = D(S31);
#endif

////////////////////////////////////////////////////////////
//////////////////////// MODULES //////////////////////////

static Touch touch;
static Bass bass;

////////////////////////////////////////////////////////////
////////////////////////// STATE ///////////////////////////

auto is_to_touched = false;
auto is_ch_touched = false;

////////////////////////////////////////////////////////////
///////////////////// TOUCH CALLBACKS //////////////////////
void OnPadTouch(uint16_t pad) {
 // Scale & Tempo
  if (pad == 0) {
    if (is_to_touched) bass.SlowDown();
    else if (is_ch_touched) bass.PrevScale();
    return;
  }
  if (pad == 2) {
    if (is_to_touched) bass.SpeedUp();
    else if (is_ch_touched) bass.NextScale();
    return;
  }

  //Mono/poly
  if (pad == 11 && is_to_touched) bass.ToggleMonoPoly();

  //Notes
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
  bass.NoteOn(pad - kFirstNotePad);
}

void OnPadRelease(uint16_t pad) {
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
  bass.NoteOff(pad - kFirstNotePad);
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
void AudioCallback(float **in, float **out, size_t size) {
  bass.Process(out, size);
}

///////////////////////////////////////////////////////////////
////////////////////////// SETUP //////////////////////////////
void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  DAISY.SetAudioBlockSize(4);
  float sample_rate = DAISY.AudioSampleRate();
  float buffer_size = DAISY.AudioBlockSize();

  bass.Init(sample_rate, buffer_size);

  touch.Init();
  touch.SetOnTouch(OnPadTouch);
  touch.SetOnRelease(OnPadRelease);

  arp_mode_switch.Init();
  osc2_mode_switch.Init();

  pinMode(LED_BUILTIN, OUTPUT);

  #ifdef EXTERNAL_SYNC
  pinMode(clk_pin, INPUT);
  #endif

  DAISY.begin(AudioCallback);
}

///////////////////////////////////////////////////////////////
////////////////////////// LOOP ///////////////////////////////
Bass::VoxParams v_params;
Bass::FilterParams f_params;
void loop() {
  #ifdef EXTERNAL_SYNC
  bass.ProcessClockIn(!digitalRead(clk_pin));
  #endif

  touch.Process();

  is_to_touched = touch.IsTouched(10);
  is_ch_touched = touch.IsTouched(11);

  auto arp_mode_value = arp_mode_switch.Value();
  bass.SetArpOn(arp_mode_value > 0);
  bass.SetLatch(arp_mode_value > 1);

  auto osc1_value = osc1_mult_knob.Process();
  osc_1_freq.SetActive(!is_to_touched, osc1_value);
  osc_1_shape.SetActive(is_to_touched, osc1_value);
  if (is_to_touched) osc_1_shape.Process(osc1_value);
  else osc_1_freq.Process(osc1_value);

  v_params.osc1_pitch = osc_1_freq.Value();
  v_params.osc1_shape = osc_1_shape.Value();
  v_params.osc2_pitch = osc2_mult_knob.Process();
  v_params.osc2_mode_index = osc2_mode_switch.Value(); 
  v_params.osc2_amnt = fmap(osc2_amount.Process(), 0.f, 1.f, Mapping::EXP);
  v_params.env = env_fader.Process();
  bass.SetVoxParams(v_params);

  bass.SetPattern(pattern_knob.Process());
  bass.SetHumanNoteChance(human_notes_knob.Process());

  auto flt_value = fmap(filter_fader.Process(), 0.f, 1.f, Mapping::EXP);
  flt_freq.SetActive(!is_to_touched && !is_ch_touched, flt_value);
  flt_reso.SetActive(is_to_touched, flt_value);
  flt_env_amount.SetActive(is_ch_touched, flt_value);
  
  auto human_verb_knob_value = human_verb_knob.Process();
  verb_value.SetActive(is_to_touched, human_verb_knob_value);
  human_env_value.SetActive(!is_to_touched, human_verb_knob_value);

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

  f_params.freq = flt_freq.Value();
  f_params.reso = flt_reso.Value();
  f_params.env_amount = flt_env_amount.Value();
  bass.SetFilterParams(f_params);

  bass.SetHumanEnvelopeChance(human_env_value.Value());
  
  bass.SetReverbMix(verb_value.Value());  

  digitalWrite(LED_BUILTIN, bass.IsLatched());

  delay(4);
}
