#include "simple-daisy.h"
#include "trigger.h"
#include "simplebd.h"
#include "simplesd.h"
#include "simplehh.h"
#include "trk.h"
#include "term.h"
#include "click.h"
#include "aknob.h"

using namespace synthux;

static Metro metro;
static Trigger trig;
static Terminal term;

static Track bd_track;
static Track sd_track;
static Track hh_track;

static Click clck;
static SimpleBD bd;
static SimpleSD sd;
static SimpleHH hh;

static AKnob bd_knob(A(S30));
static AKnob sd_knob(A(S33));
static AKnob hh_knob(A(S34));
static AKnob speed_knob(A(S37));
static AKnob swing_knob(A(S36));

static const uint16_t kPlayStopPad = 0;
static const uint16_t kRecordPad = 2;
static const uint16_t kBDPad = 3;
static const uint16_t kSDPad = 4;
static const uint16_t kHHPad = 6;
static const uint16_t kClckPad = 7;
static const uint16_t kDeletePad = 8;


bool clck_on = false;
size_t clck_cnt = 0;
bool is_playing = false;
bool is_recording = false;
bool is_clearing = false;

const size_t clck_sycle = 2 * Trigger::kPPQN - 1;
bool blink = false;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = Trigger::kPPQN * kBPMRange / kSecPerMin;

bool bd_trig = false;
bool sd_trig = false; 
bool hh_trig = false;
bool clck_trig = false;

float bd_snd = .5f;
float sd_snd = .5f;
float hh_snd = .5f;

void OnTapPad(uint16_t pad) {
  switch (pad) {
    case kPlayStopPad: ToggleIsPlaying(); break;
    case kBDPad: if (!is_clearing) { bd_track.Hit(); bd_trig = true; } break;
    case kSDPad: if (!is_clearing) { sd_track.Hit(); sd_trig = true; } break;
    case kHHPad: if (!is_clearing) { hh_track.Hit(); hh_trig = true; } break;
    case kRecordPad: ToggleRecording(); break;
    case kClckPad: ToggleClck(); break;
  };
}

void ToggleIsPlaying() {
  is_playing = !is_playing;
  if (is_playing) {
    bd_track.Reset();
    sd_track.Reset();
    hh_track.Reset();
    trig.Reset();
  }
}

void ToggleRecording() {
  is_recording = !is_recording;
  bd_track.SetRecording(is_recording);
  sd_track.SetRecording(is_recording);
  hh_track.SetRecording(is_recording);
  if (is_recording) is_playing = true;
}

void ToggleClck() {
  clck_on = !clck_on;
}

float output = 0;
void AudioCallback(float **in, float **out, size_t size) {  
  for (size_t i = 0; i < size; i++) {
    auto t = metro.Process();
    if (is_playing) {
      if (t) {
        if (clck_cnt-- == 0) {
          blink = true;
          clck_trig = true;
          clck_cnt = clck_sycle;
        }
        else {
          blink = false;
        }
        if (trig.Tick()) {
          if (bd_track.Tick()) {
            bd_trig = true;
            bd_snd = bd_track.Sound();
          }
          if (sd_track.Tick()) {
            sd_trig = true;
            sd_snd = sd_track.Sound();  
          }
          if (hh_track.Tick()) {
            hh_trig = true;
            hh_snd = hh_track.Sound();  
          }
        }
      }
    }
    if (bd_trig) bd.SetSound(bd_snd);
    if (sd_trig) sd.SetSound(sd_snd);
    if (hh_trig) hh.SetSound(hh_snd);
    output = bd.Process(bd_trig) + sd.Process(sd_trig) * 0.4 + hh.Process(hh_trig) * 0.5;
    if (clck_on) output += clck.Process(clck_trig) * 0.7;
    bd_trig = false;
    sd_trig = false;  
    hh_trig = false;
    clck_trig = false; 
    out[0][i] = out[1][i] = output;
  }
}

void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();

  Serial.begin(9600);

  metro.Init(96, sample_rate);

  bd.Init(sample_rate);
  sd.Init(sample_rate);
  hh.Init(sample_rate);

  clck.Init(sample_rate);

  term.Init();
  term.SetOnTap(OnTapPad);

  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

void loop() {
  auto speed = 2 * speed_knob.Process();
  auto freq = kMinFreq + kFreqRange * speed;
  metro.SetFreq(freq);

  trig.SetSwing(swing_knob.Process());

  term.Process();

  is_clearing = term.IsTouched(kDeletePad);
  bd_track.SetClearing(is_clearing && term.IsTouched(kBDPad));
  sd_track.SetClearing(is_clearing && term.IsTouched(kSDPad));
  hh_track.SetClearing(is_clearing && term.IsTouched(kHHPad));

  bd_snd = bd_knob.Process();
  bd_track.SetSound(bd_snd);
  sd_snd = sd_knob.Process();
  sd_track.SetSound(sd_snd);
  hh_snd = hh_knob.Process();
  hh_track.SetSound(hh_snd);

  if (is_recording) {
    digitalWrite(LED_BUILTIN, blink);
  }
}
