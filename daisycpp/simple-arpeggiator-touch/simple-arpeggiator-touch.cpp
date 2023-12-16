
#include "vox.h"
#include "arp.h"
#include "term.h"
#include "scale.h"

using namespace synthux;

////////////////////////////////////////////////////////////
////////////////////////// CONTROLS ////////////////////////

#define S30 D15 // SWITCH : ORDERED / AS PLAYED
#define S31 A1  // KNOB : SPEED
#define S32 A2  // KNOB : LENGTH
#define S33 A3  // KNOB : DIRECTION / RANDOM

static const int kAnalogResolution  = 7; //7bits => 0..127
static const float kKnobMax = powf(2.f, kAnalogResolution) - 1.f;

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////

static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;

static Scale scale;
static Terminal term;
static Arp<kNotesCount, kPPQN> arp;
static Metro metro;
static Vox vox;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = kPPQN * kBPMRange / kSecPerMin;

////////////////////////////////////////////////////////////
///////////////////// CALLBACKS ////////////////////////////

void OnScaleSelect(uint8_t index) { scale.SetScaleIndex(index); }
void OnTerminalNoteOn(uint8_t num, uint8_t vel) { arp.NoteOn(num, vel); }
void OnTerminalNoteOff(uint8_t num) { arp.NoteOff(num); }
void OnArpNoteOn(uint8_t num, uint8_t vel) { vox.NoteOn(scale.FreqAt(num), vel / 127.f); }
void OnArpNoteOff(uint8_t num) { vox.NoteOff(); }

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (metro.Process()) arp.Trigger();
    out[0][i] = out[1][i] = vox.Process();
  }
}

///////////////////////////////////////////////////////////////
////////////////////////// SETUP //////////////////////////////

void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sample_rate = DAISY.get_samplerate();

  vox.Init(sample_rate);

  metro.Init(48, sample_rate); //48Hz = 24ppqn @ 120bpm 

  term.Init();
  term.SetOnNoteOn(OnTerminalNoteOn);
  term.SetOnNoteOff(OnTerminalNoteOff);
  term.SetOnScaleSelect(OnScaleSelect);

  arp.SetOnNoteOn(OnArpNoteOn);
  arp.SetOnNoteOff(OnArpNoteOff);
  
  pinMode(S30, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  analogReadResolution(kAnalogResolution);

  DAISY.begin(AudioCallback);
}

///////////////////////////////////////////////////////////////
////////////////////////// LOOP ///////////////////////////////

void loop() {
  auto speed = analogRead(S31) / kKnobMax;
  auto freq = kMinFreq + kFreqRange * speed;
  
  metro.SetFreq(freq); 

  digitalWrite(LED_BUILTIN, term.IsLatched());
  
  term.Process();

  auto arp_lgt = analogRead(S32) / kKnobMax;
  auto arp_ctr = analogRead(S33) / kKnobMax;
  auto arp_dir = arp_ctr < .5f ? ArpDirection::fwd : ArpDirection::rev;
  auto arp_rnd = arp_ctr < .5f ? 2.f * arp_ctr : 2.f * (1.f - arp_ctr);
  arp.SetDirection(arp_dir);
  arp.SetRandChance(arp_rnd);
  arp.SetAsPlayed(digitalRead(S30));
  arp.SetNoteLength(arp_lgt);

  delay(4);
}
