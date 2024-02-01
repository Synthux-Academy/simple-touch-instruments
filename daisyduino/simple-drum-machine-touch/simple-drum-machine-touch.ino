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
  Pan = 0,
  Timbre,
  Volume,
  kParamCount
};

static AKnob bd_knob(A(S32));
static AKnob sd_knob(A(S34));
static AKnob hh_knob(A(S33));
static array<AKnob<>, kDrumCount> drum_knobs = { bd_knob, sd_knob, hh_knob };

static array<MValue, kParamCount> bd_val;
static array<MValue, kParamCount> sd_val;
static array<MValue, kParamCount> hh_val;
static array<array<MValue, kParamCount>, kDrumCount> m_val = { bd_val, sd_val, hh_val };

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
static Trigger<kPPQN, Every::_32th> trigger;

static Track bd_track;
static Track sd_track;
static Track hh_track;

static Click click;
static SimpleBD bd;
static SimpleSD sd;
static SimpleHH hh;

///////////////////////////////////////////////////////////////
//////////////////////// VARIABLES ////////////////////////////
static constexpr float kTimbreOffset = 0.09;
bool trig[kDrumCount] = { false, false, false };
float timbres[kDrumCount] = { 0.5f, 0.5f, 0.5f };
float timbresA[kDrumCount] = { 0.41f, 0.41f, 0.41f }; //timbres - timbre offset
float timbresB[kDrumCount] = { 0.59f, 0.59f, 0.59f }; //timbres + timbre offset
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
    case kBDPadA: if (!is_clearing) { timbres[BD] = timbresA[BD]; bd_track.HitStroke(timbres[BD]); trig[BD] = true; } break;
    case kBDPadB: if (!is_clearing) { timbres[BD] = timbresB[BD]; bd_track.HitStroke(timbres[BD]); trig[BD] = true; } break;
    case kSDPadA: if (!is_clearing) { timbres[SD] = timbresA[SD]; sd_track.HitStroke(timbres[SD]); trig[SD] = true; } break;
    case kSDPadB: if (!is_clearing) { timbres[SD] = timbresB[SD]; sd_track.HitStroke(timbres[SD]); trig[SD] = true; } break;
    case kHHPadA: if (!is_clearing) { timbres[HH] = timbresA[HH]; hh_track.HitStroke(timbres[HH]); trig[HH] = true; } break;
    case kHHPadB: if (!is_clearing) { timbres[HH] = timbresB[HH]; hh_track.HitStroke(timbres[HH]); trig[HH] = true; } break;
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

  if (bd_track.Tick()) { trig[BD] = true; timbres[BD] = bd_track.AutomationValue(); }
  if (sd_track.Tick()) { trig[SD] = true; timbres[SD] = sd_track.AutomationValue(); }
  if (hh_track.Tick()) { trig[HH] = true; timbres[HH] = hh_track.AutomationValue(); }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float click_out;
float drum_out[kDrumCount];
void AudioCallback(float **in, float **out, size_t size) {  
  //Advance clock
  clck.Tick();

  //Set timbre
  if (trig[BD]) bd.SetSound(timbres[BD]);
  if (trig[SD]) sd.SetSound(timbres[SD]);
  if (trig[HH]) hh.SetSound(timbres[HH]);
  
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
    m_val[i][Pan].Init(0.5f);
    m_val[i][Timbre].Init(0.5f);
    m_val[i][Volume].Init(1.0f);
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
float knob_val, pan, volume, timbre, pan0, pan1, tempo;
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
  
  auto mode = knob_mode_switch.Value(); //0 -> Pan, 1 -> Timbre, 2 -> Volume
  for (auto i = 0; i < kDrumCount; i++) {
    knob_val = drum_knobs[i].Process();
    auto& val = m_val[i];
    val[Pan].SetActive(mode == Pan, knob_val);
    val[Timbre].SetActive(mode == Timbre, knob_val);
    val[Volume].SetActive(mode == Volume, knob_val);
    switch (mode) {
    case Timbre: //timbre
      timbre = fmap(val[Timbre].Process(knob_val), 0.1, 0.9);
      timbresA[i] = timbre - kTimbreOffset;
      timbresB[i] = timbre + kTimbreOffset;
      break;

    default: //Volume or Pan
      volume = fmap(val[Volume].Process(knob_val), 0.f, 1.f, Mapping::EXP) * mix_kof[i];
      pan = val[Pan].Process(knob_val);
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
