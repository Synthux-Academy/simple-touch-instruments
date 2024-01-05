#include "simple-daisy-touch.h"
#include "trigger.h"
#include "simplebd.h"
#include "simplesd.h"
#include "simplehh.h"
#include "trk.h"
#include "clk.h"
#include "click.h"
#include "aknob.h"
#include "multivalue.h"

using namespace synthux;

static const int kDrumCount = 3;
static const int kParamCount = 3; //pan, volume, timbre
static std::array<MultiValue<kParamCount>, kDrumCount> m_val;
static AKnob bd_knob(A(S32));
static AKnob sd_knob(A(S33));
static AKnob hh_knob(A(S34));
static AKnob drum_knobs[kDrumCount] = { bd_knob, sd_knob, hh_knob };

static AKnob tempo_knob(A(S30));
static AKnob swing_knob(A(S36));

static int func_switch_a = D(S09);
static int func_switch_b = D(S10);

static simpletouch::Touch touch;
static const uint16_t kRecordPad = 0;
static const uint16_t kPlayStopPad = 2;
static const uint16_t kBDPad = 3;
static const uint16_t kSDPad = 4;
static const uint16_t kHHPad = 6;
static const uint16_t kClickPad = 7;
static const uint16_t kClearingPad = 8;

// Comment this if you're not
// planning using external sync
#define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clck_pin = D(S31);
#endif

static constexpr size_t kPPQN = 48;

static Clock<kPPQN> clck;
static Trigger<kPPQN, Every::_32th> trig;

static Track bd_track;
static Track sd_track;
static Track hh_track;
static Track* tracks[kDrumCount] = { &bd_track, &sd_track, &hh_track };

static Click click;
static SimpleBD bd;
static SimpleSD sd;
static SimpleHH hh;


float timbres[kDrumCount] = { 0.5f, 0.5f, 0.5f };
float mix_kof[kDrumCount] = { 1.0f, 0.4f, 0.5f }; //bd, sd, hh
float mix_volume[kDrumCount][2] = { 
  { mix_kof[0], mix_kof[0] }, //bd
  { mix_kof[1], mix_kof[1] }, //sd
  { mix_kof[2], mix_kof[2] } //hh
};

bool bd_trig = false;
bool sd_trig = false; 
bool hh_trig = false;

bool ck_trig = false;
size_t click_cnt = 0;
bool click_on = false;
bool blink = false;

bool is_recording = false;
bool is_clearing = false;

void OnPadTouch(uint16_t pad) {
  switch (pad) {
    case kPlayStopPad: ToggleClock(); break;
    case kBDPad: if (!is_clearing) { bd_track.Hit(); bd_trig = true; } break;
    case kSDPad: if (!is_clearing) { sd_track.Hit(); sd_trig = true; } break;
    case kHHPad: if (!is_clearing) { hh_track.Hit(); hh_trig = true; } break;
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
    trig.Reset();
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

auto cnt = 0;
void OnClockTick() {
  if (click_cnt-- == 0) {
    blink = true;
    ck_trig = true;
    click_cnt = kPPQN - 1;
  }
  else {
    blink = false;
  }
  
  if (!trig.Tick()) return;

  if (bd_track.Tick()) { bd_trig = true; timbres[0] = bd_track.Sound(); }
  if (sd_track.Tick()) { sd_trig = true; timbres[1] = sd_track.Sound(); }
  if (hh_track.Tick()) { hh_trig = true; timbres[2] = hh_track.Sound(); }
}

float click_out;
float drum_out[kDrumCount];
void AudioCallback(float **in, float **out, size_t size) {  
  clck.Tick();
  if (bd_trig) bd.SetSound(timbres[0]);
  if (sd_trig) sd.SetSound(timbres[1]);
  if (hh_trig) hh.SetSound(timbres[2]);
  
  for (auto i = 0; i < size; i++) {  
    out[0][i] = out[1][i] = 0;
    drum_out[0] = bd.Process(bd_trig);
    drum_out[1] = sd.Process(sd_trig); 
    drum_out[2] = hh.Process(hh_trig);
    for (auto k = 0; k < kDrumCount; k++) {
      out[0][i] += drum_out[k] * mix_volume[k][0];
      out[1][i] += drum_out[k] * mix_volume[k][1];  
    }
    click_out = click.Process(ck_trig) * 0.7;
    if (click_on) {
      out[0][i] += click_out;
      out[1][i] += click_out;
    }
    bd_trig = false;
    sd_trig = false;
    hh_trig = false;
    ck_trig = false;
  }
}

void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.AudioSampleRate();
  float buffer_size = DAISY.AudioBlockSize();

  Serial.begin(9600);

  clck.Init(sample_rate, buffer_size);
  clck.SetOnTick(OnClockTick);
  #ifdef EXTERNAL_SYNC
  pinMode(clck_pin, INPUT);
  #endif

  bd.Init(sample_rate);
  sd.Init(sample_rate);
  hh.Init(sample_rate);

  for (auto i = 0; i < kDrumCount; i++) {
    m_val[i].Init({ 0.5f, 0.5f, 1.0f }); //{ pan, timbre, volume }
    tracks[i]->SetSound(0.5f);
  }

  click.Init(sample_rate);

  touch.Init();
  touch.SetOnTouch(OnPadTouch);

  pinMode(func_switch_a, INPUT_PULLUP);
  pinMode(func_switch_b, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

float knob_val, pan, volume, timbre, pan0, pan1, tempo;
uint8_t func_a_val, func_b_val, func;
void loop() {
  tempo = tempo_knob.Process();
  clck.SetTempo(tempo);

  #ifdef EXTERNAL_SYNC
  clck.Process(!digitalRead(clck_pin));
  #endif

  trig.SetSwing(swing_knob.Process());

  touch.Process();

  is_clearing = touch.IsTouched(kClearingPad);
  bd_track.SetClearing(is_clearing && touch.IsTouched(kBDPad));
  sd_track.SetClearing(is_clearing && touch.IsTouched(kSDPad));
  hh_track.SetClearing(is_clearing && touch.IsTouched(kHHPad));

  func_a_val = static_cast<uint8_t>(digitalRead(func_switch_a));
  func_b_val = static_cast<uint8_t>(!digitalRead(func_switch_b));
  func = func_a_val + func_b_val; //0 - pan, 1 - timebre, 2 - volume
  
  for (auto i = 0; i < kDrumCount; i++) {
    knob_val = drum_knobs[i].Process();
    auto& val = m_val[i];
    val.SetActive(func, knob_val);
    val.Process(knob_val);
    switch (func) {
    case 1: //timbre
      tracks[i]->SetSound(val.Value(1));
      timbres[i] = val.Value(1);
      break;

    default: //volume or pan
      volume = fmap(val.Value(2), 0.f, 1.f, Mapping::EXP) * mix_kof[i];
      pan = val.Value(0);
      pan0 = 1.f;
      pan1 = 1.f;  
      if (pan < 0.47f) pan1 = 2.f * pan;
      else if (pan > 0.53f) pan0 = 2.f * (1.f - pan);
      mix_volume[i][0] = volume * pan0;
      mix_volume[i][1] = volume * pan1;
  }
  }

  if (is_recording) {
    digitalWrite(LED_BUILTIN, blink);
  }

  delay(4);
}
