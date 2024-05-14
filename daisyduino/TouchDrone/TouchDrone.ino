#include "simple-daisy.h"
#include "vox.h"
#include "term.h"
#include "driver.h"
#include "aknob.h"
#include "memknob.h"
#include "flt.h"
#include "env.h"
#include <array>

static std::array<synthux::Vox, synthux::Driver::kVoices> vox;

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

static synthux::Terminal terminal;
static synthux::AKnob freq_knob(A(S30));
static synthux::AKnob spread_knob(A(S32));
static synthux::AKnob glide_knob(A(S33));
static synthux::AKnob envelope_knob(A(S36));
static synthux::AKnob filter_knob(A(S37));
static int quantize_switch = D(S07);
static int scale_a_switch = D(S09);
static int scale_b_switch = D(S10);

static synthux::Driver drive;
static synthux::Filter filter;
static synthux::Envelope envelope;

static const int kPadsCount = 7;
static std::array<synthux::MemKnob, kPadsCount> mk_freq;
static std::array<synthux::MemKnob, kPadsCount> mk_spread;

bool gate = false;

void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    float output = 0;
    if (envelope.IsRunning() || gate) {
      for (auto& v: vox) output += v.Process(); 
      output = filter.Process(output) * envelope.Process(gate) * 0.75;
    }
    out[0][i] = out[1][i] = output;
  }
}

void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sampleRate = DAISY.get_samplerate();

  envelope.Init(sampleRate);
  terminal.Init();
  filter.Init(sampleRate);
  for (auto& v: vox) v.Init(sampleRate);
  for (auto i = 0; i < kPadsCount; i++) {
    mk_freq[i].Init(static_cast<float>(i) / kPadsCount);
  }
  
  for (auto& k: mk_spread) k.Init(0);

  pinMode(quantize_switch, INPUT_PULLUP);
  pinMode(scale_a_switch, INPUT_PULLUP);
  pinMode(scale_b_switch, INPUT_PULLUP);

  // BEGIN CALLBACK
  DAISY.begin(AudioCallback);
}

void loop() {
  auto freq = freq_knob.Process();
  auto spread = spread_knob.Process();
  terminal.Process();  
  
  gate = false;

  auto scale_index = 1;
  auto scale_a_on = digitalRead(scale_a_switch);
  auto scale_b_on = digitalRead(scale_b_switch);
  if (scale_a_on == scale_b_on) scale_index = 1;
  else if (scale_a_on) scale_index = 2;
  else scale_index = 0;

  drive.SetScaleIndex(scale_index);

  for (auto i = 0; i < kPadsCount; i++) {
    auto is_touched = terminal.IsTouched(i + 3); //pads 3-9
    mk_freq[i].SetActive(is_touched, freq);
    mk_spread[i].SetActive(is_touched, spread);
    if (is_touched) {
      gate = true;
      drive.SetPitchAndSpread(mk_freq[i].Process(freq), mk_spread[i].Process(spread), digitalRead(quantize_switch));
    }
  }

  for (auto i = 0; i < vox.size(); i++) {
    vox[i].SetPortamento(1 - glide_knob.Process());
    vox[i].SetFreq(drive.FreqAt(i));
  }

  filter.SetTimbre(filter_knob.Process());
  envelope.SetAmount(envelope_knob.Process());

  delay(4);
}
