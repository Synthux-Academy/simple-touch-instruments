// SYNTHUX ACADEMY /////////////////////////////////////////
// DRUM MACHINE ////////////////////////////////////////////
#include "simple-daisy-touch.h"
#include "aknob.h"
#include "onoffon.h"
#include "mvalue.h"
#include "syncclock.h"
#include "trigger.h"
#include "track.h"
#include "simplebd.h"
#include "simplesd.h"
#include "simplehh.h"
#include "click.h"

using namespace synthux;
using namespace simpletouch;
using namespace std;

////////////////////////////////////////////////////////////
////////////// KNOBS, SWITCHES, PADS, JACKS ////////////////
enum Drum {
  BD = 0,
  SD,
  HH,
  kDrumCount
};

enum Param {
  pPan = 0,
  pTone,
  pVolume,
  kParamCount
};

static AKnob bd_knob(A(S32));
static AKnob sd_knob(A(S34));
static AKnob hh_knob(A(S33));
static array<AKnob<>, kDrumCount> drum_knobs = { bd_knob, sd_knob, hh_knob };

///////////////////////////////////////
// Every drum has 3 parameters... /////
static array<MValue, kParamCount> bd_val;
static array<MValue, kParamCount> sd_val;
static array<MValue, kParamCount> hh_val;
// ...this gives an array of three arrays:
// [
//    [BD Pan, BD Tone, BD Volume],
//    [SD Pan, SD Tone, SD Volume],
//    [HH Pan, HH Tone, HH Volume],
// ]
static array<array<MValue, kParamCount>, kDrumCount> m_val = { bd_val, sd_val, hh_val };
////////////////////////////////////////

static AKnob tempo_knob(A(S30));
static AKnob swing_knob(A(S36));

static OnOffOn knob_mode_switch(D(S09), D(S10));

static Touch touch;

static const uint16_t kClickPad = 0;
static const uint16_t kPlayStopPad = 2;
static const uint16_t kBDPadA = 3;
static const uint16_t kBDPadB = 4;
static const uint16_t kSDPadA = 6;
static const uint16_t kSDPadB = 7;
static const uint16_t kHHPadA = 8;
static const uint16_t kHHPadB = 9;
static const uint16_t kRecordPad = 10;
static const uint16_t kClearingPad = 11;

// Comment this if you're not
// planning using external sync
#define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clock_pin = D(S31);
#endif

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static constexpr size_t kPPQN = 48;

static SyncClock<kPPQN> clck;
static Trigger trigger(kPPQN, Every::_32th);

static Track bd_track;
static Track sd_track;
static Track hh_track;

static Click click;
static SimpleBD bd;
static SimpleSD sd;
static SimpleHH hh;

///////////////////////////////////////////////////////////////
//////////////////////// VARIABLES ////////////////////////////
static constexpr float kToneOffset = 0.09;
bool trig[kDrumCount] = { false, false, false };
float tones[kDrumCount] = { 0.5f, 0.5f, 0.5f };
float tonesA[kDrumCount] = { 0.41f, 0.41f, 0.41f }; //tones - timbre offset
float tonesB[kDrumCount] = { 0.59f, 0.59f, 0.59f }; //tones + timbre offset
float mix_kof[kDrumCount] = { 1.0f, 0.4f, 0.5f }; //bd, sd, hh
float mix_volume[kDrumCount][2] = { 
  { mix_kof[BD], mix_kof[BD] },
  { mix_kof[SD], mix_kof[SD] },
  { mix_kof[HH], mix_kof[HH] }
};

bool click_trig = false;
size_t click_cnt = 0;
bool click_on = false;
bool blink = false;

bool is_recording = false;
bool is_clearing = false;

///////////////////////////////////////////////////////////////
////////////////////// TOUCH HANDLERS /////////////////////////
void OnPadTouch(uint16_t pad) {
  switch (pad) {
    case kPlayStopPad: ToggleClock(); break;
    case kBDPadA: if (!is_clearing) { tones[BD] = tonesA[BD]; bd_track.HitStroke(tones[BD]); trig[BD] = true; } break;
    case kBDPadB: if (!is_clearing) { tones[BD] = tonesB[BD]; bd_track.HitStroke(tones[BD]); trig[BD] = true; } break;
    case kSDPadA: if (!is_clearing) { tones[SD] = tonesA[SD]; sd_track.HitStroke(tones[SD]); trig[SD] = true; } break;
    case kSDPadB: if (!is_clearing) { tones[SD] = tonesB[SD]; sd_track.HitStroke(tones[SD]); trig[SD] = true; } break;
    case kHHPadA: if (!is_clearing) { tones[HH] = tonesA[HH]; hh_track.HitStroke(tones[HH]); trig[HH] = true; } break;
    case kHHPadB: if (!is_clearing) { tones[HH] = tonesB[HH]; hh_track.HitStroke(tones[HH]); trig[HH] = true; } break;
    case kRecordPad: ToggleRecording(); break;
    case kClickPad: ToggleClick(); break;
  };
}

void ToggleClock() {
  if (clck.IsRunning()) {
    clck.Stop();
    bd_track.Reset();
    sd_track.Reset();
    hh_track.Reset();
    trigger.Reset();
    click.Reset();
    click_cnt = 0;
  }
  else {
    clck.Run();
  }
}

void ToggleRecording() {
  is_recording = !is_recording;
  bd_track.SetRecording(is_recording);
  sd_track.SetRecording(is_recording);
  hh_track.SetRecording(is_recording);
}

void ToggleClick() {
  click_on = !click_on;
}

///////////////////////////////////////////////////////////////
////////////////////// CLOCK HANDLERS /////////////////////////
auto cnt = 0;
void OnClockTick() {
  if (click_cnt-- == 0) {
    blink = true;
    click_trig = true;
    click_cnt = kPPQN - 1;
  }
  else {
    blink = false;
  }
  
  if (!trigger.Tick()) return;

  if (bd_track.Tick()) { trig[BD] = true; tones[BD] = bd_track.AutomationValue(); }
  if (sd_track.Tick()) { trig[SD] = true; tones[SD] = sd_track.AutomationValue(); }
  if (hh_track.Tick()) { trig[HH] = true; tones[HH] = hh_track.AutomationValue(); }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float click_out;
float drum_out[kDrumCount];
void AudioCallback(float **in, float **out, size_t size) {  
  //Advance clock
  clck.Tick();

  //Set timbre
  if (trig[BD]) bd.SetTone(tones[BD]);
  if (trig[SD]) sd.SetTone(tones[SD]);
  if (trig[HH]) hh.SetTone(tones[HH]);
  
  for (auto i = 0; i < size; i++) {
    // Zero out as there's no guarantee that the buffer
    // is supplied with zero values.
    out[0][i] = out[1][i] = 0;
    
    //Mix drum tracks
    drum_out[BD] = bd.Process(trig[BD]);
    drum_out[SD] = sd.Process(trig[SD]); 
    drum_out[HH] = hh.Process(trig[HH]);
    for (auto k = 0; k < kDrumCount; k++) {
      out[0][i] += drum_out[k] * mix_volume[k][0];
      out[1][i] += drum_out[k] * mix_volume[k][1];  
      trig[k] = false;
    }
    
    //Mix click
    if (click_on) {
      click_out = click.Process(click_trig) * 0.7;
      out[0][i] += click_out;
      out[1][i] += click_out;
      click_trig = false;
    }
  }
}

///////////////////////////////////////////////////////////////
///////////////////////// SETUP ///////////////////////////////
void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.AudioSampleRate();
  float buffer_size = DAISY.AudioBlockSize();

  clck.Init(sample_rate, buffer_size);
  clck.SetOnTick(OnClockTick);
  #ifdef EXTERNAL_SYNC
  pinMode(clock_pin, INPUT);
  #endif

  bd.Init(sample_rate);
  sd.Init(sample_rate);
  hh.Init(sample_rate);

  for (auto i = 0; i < kDrumCount; i++) {
    drum_knobs[i].Init();
    m_val[i][pPan].Init(0.5f);
    m_val[i][pTone].Init(0.5f);
    m_val[i][pVolume].Init(1.0f);
  }

  click.Init(sample_rate);

  touch.Init();
  touch.SetOnTouch(OnPadTouch);

  knob_mode_switch.Init();
  
  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

///////////////////////////////////////////////////////////////
///////////////////////// LOOP ////////////////////////////////
float knob_val, pan, volume, drum_tone, pan0, pan1, tempo;
uint8_t func_a_val, func_b_val, func;
void loop() {
  tempo = tempo_knob.Process();
  clck.SetTempo(tempo);

  #ifdef EXTERNAL_SYNC
  clck.Process(!digitalRead(clock_pin));
  #endif

  trigger.SetSwing(swing_knob.Process());

  touch.Process();

  is_clearing = touch.IsTouched(kClearingPad);
  bd_track.SetClearing(is_clearing && (touch.IsTouched(kBDPadA) || touch.IsTouched(kBDPadB)));
  sd_track.SetClearing(is_clearing && (touch.IsTouched(kSDPadA) || touch.IsTouched(kSDPadB)));
  hh_track.SetClearing(is_clearing && (touch.IsTouched(kHHPadA) || touch.IsTouched(kHHPadB)));
  
  auto mode = knob_mode_switch.Value(); //0 -> pPan, 1 -> pTone, 2 -> pVolume
  for (auto i = 0; i < kDrumCount; i++) {
    knob_val = drum_knobs[i].Process();
    auto& val = m_val[i];
    val[pPan].SetActive(mode == pPan, knob_val);
    val[pTone].SetActive(mode == pTone, knob_val);
    val[pVolume].SetActive(mode == pVolume, knob_val);
    switch (mode) {
    case pTone:
      drum_tone = fmap(val[pTone].Process(knob_val), 0.1, 0.9);
      tonesA[i] = drum_tone - kToneOffset;
      tonesB[i] = drum_tone + kToneOffset;
      break;

    default: //pVolume or pPan
      volume = fmap(val[pVolume].Process(knob_val), 0.f, 1.f, Mapping::EXP) * mix_kof[i];
      pan = val[pPan].Process(knob_val);
      pan0 = 1.f;
      pan1 = 1.f;  
      if (pan < 0.47f) pan1 = 2.f * pan;
      else if (pan > 0.53f) pan0 = 2.f * (1.f - pan);
      mix_volume[i][0] = volume * pan0;
      mix_volume[i][1] = volume * pan1;
  }
  }

  digitalWrite(LED_BUILTIN, is_recording && blink || is_clearing);

  delay(4);
}
