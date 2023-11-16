#include "simple-daisy.h"
#include "trigger.h"
#include "cpattern.h"
#include "multiknob.h"
#include "simplebd.h"
#include "simplesd.h"
#include "trk.h"
#include "term.h"
#include "click.h"

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
static HiHat<> hh;

static const int kRunPin = D(S30);
static const int kSwitchPin = D(S10);
static const int kSpeedPin = A(S37);
static const int kPatternPin = A(S32);
static const int kShiftPin = A(S33);
static MultiKnob<2> speed_swing_knob({ 512, 0 }); //Tempo 140, Swing 0
static MultiKnob<3> pattern_knob({ 512, 128, 1023 }); //BD 4 onsets, SD 2 onsets, HH 16 onsets
static MultiKnob<3> shift_knob({ 0, 256, 0 }); //SD 1/4th shift

float output = 0;
bool is_playing = false; 
size_t clck_cnt = 0;
bool clck_trig = false;
const size_t clck_sycle = 2 * Trigger::kPPQN - 1;
bool blink = false;

bool bd_trig = false;
bool sd_trig = false; 
bool hh_trig = false;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = Trigger::kPPQN * kBPMRange / kSecPerMin;

void OnTerminalNoteOn(uint8_t num, uint8_t vel) {
  if (num == 3) bd_track.Hit();
  else if (num == 4) sd_track.Hit();
  else if (num == 6) hh_track.Hit();
} 

void OnTerminalNoteOff(uint8_t num) {

}

void AudioCallback(float **in, float **out, size_t size) {  
  for (size_t i = 0; i < size; i++) {
    output = 0;
    bd_trig = false;
    sd_trig = false;  
    hh_trig = false;    
    auto t = metro.Process();
    if (t) {
      if (is_playing) {
        if (clck_cnt-- == 0) {
          blink = true;
          clck_trig = true;
          clck_cnt = clck_sycle;
        }
        else {
          blink = false;
        }
        if (trig.Tick()) {
          bd_trig = bd_track.Tick();
          sd_trig = sd_track.Tick();
          hh_trig = hh_track.Tick();
        }
      }
    }
    if (is_playing) output = bd.Process(bd_trig) + sd.Process(sd_trig) * 0.6 + hh.Process(hh_trig) * 0.5 + clck.Process(clck_trig) * 0.7;
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

  clck.Init(sample_rate);

  hh.Init(sample_rate);
  hh.SetDecay(0.7);
  hh.SetTone(0.8);
  hh.SetNoisiness(0.7);

  term.Init();
  term.SetOnNoteOn(OnTerminalNoteOn);
  term.SetOnNoteOff(OnTerminalNoteOff);

  pinMode(kSwitchPin, INPUT_PULLUP);
  pinMode(kRunPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

void SetIsPlaying(bool value) {
  // if (value && !is_playing) {
  //   trig.Reset();
  //   bd_pattern.Reset();
  //   sd_pattern.Reset();
  //   hh_pattern.Reset();
  // }
  is_playing = value;
}

void loop() {
  SetIsPlaying(digitalRead(kRunPin));

  bool mode_switch = !digitalRead(kSwitchPin);

  speed_swing_knob.Process(analogRead(kSpeedPin), mode_switch);
  auto speed = 2 * speed_swing_knob.ValueAt(0) / 1023.f;
  auto freq = kMinFreq + kFreqRange * speed;
  metro.SetFreq(freq);

  trig.SetSwing(speed_swing_knob.ValueAt(1) / 1023.f);

  term.Process();

  // pattern_knob.Process(analogRead(kPatternPin), mode_switch);
  // bd_pattern.SetOnsets(pattern_knob.ValueAt(0) / 1023.f);
  // sd_pattern.SetOnsets(pattern_knob.ValueAt(1) / 1023.f);
  // hh_pattern.SetOnsets(pattern_knob.ValueAt(2) / 1023.f);

  // shift_knob.Process(analogRead(kShiftPin), mode_switch);
  // bd_pattern.SetShift(shift_knob.ValueAt(0) / 1023.f);
  // sd_pattern.SetShift(shift_knob.ValueAt(1) / 1023.f);
  // hh_pattern.SetShift(shift_knob.ValueAt(2) / 1023.f);

  digitalWrite(LED_BUILTIN, blink);
}
