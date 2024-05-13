// SYNTHUX ACADEMY /////////////////////////////////////////
// SLICER //////////////////////////////////////////////////

#include "simple-daisy-touch.h"
#include "aknob.h"
#include "trig.h"
#include "gen.h"
#include "buf.h"
#include "cpattern.h"
#include "mknob.h"
#include "trigarp.h"
#include "clk.h"

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

static synthux::AKnob speed_knob(A(S30));
static synthux::AKnob pattern_knob(A(S32));
static synthux::AKnob position_knob(A(S33));
static synthux::AKnob shape_fader(A(S36));
static synthux::AKnob pitch_fader(A(S37));
static std::array<synthux::MKnob, 7> position_mem;

static const int reverse_switch = D(S07);
static const int arp_switch_a = D(S09);
static const int arp_switch_b = D(S10);

// Uncomment this if you're 
// using external sync
// #define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clk_pin = D(S31);
#endif

////////////////////////////////////////////////////////////
////////////////////////// TOUCH  //////////////////////////
static synthux::simpletouch::Touch touch;
static constexpr int kPadsUsed = 7;
static constexpr int kLowPad = 3;
static constexpr int kHighPad = kLowPad + kPadsUsed - 1; 
static constexpr int kRecordPad = 0;

static constexpr uint32_t kPPQN = 48;
///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static synthux::Buffer buf;
static synthux::Generator<kPadsUsed> gen;
static synthux::CPattern ptn;
static synthux::Trigger<kPPQN> trig;
static synthux::TrigArp<kPadsUsed> arp;
static synthux::Clock<kPPQN> clk;

///////////////////////////////////////////////////////////////
///////////////////////// CALLBACKS ///////////////////////////
void OnTouch(uint16_t pad) {
  if (pad >= kLowPad && pad <= kHighPad) {
    if (!clk.IsRunning()) clk.Run();
    arp.SetTrigger(pad - kLowPad, 127);
  }
}

void OnRelease(uint16_t pad) {
  if (pad >= kLowPad && pad <= kHighPad) {
    arp.RemoveTrigger(pad - kLowPad);
    if (!touch.HasTouch() && clk.IsRunning()) {
      clk.Stop();
      trig.Reset();
      ptn.Reset();
      arp.Reset();
    }
  }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
bool is_recording = false;

void AudioCallback(float **in, float **out, size_t size) {
  auto out0 = 0.f;
  auto out1 = 0.f;

  clk.Tick();

  for (size_t i = 0; i < size; i++) {
    if (is_recording) {
      buf.Write(in[0][i], in[1][i]); 
      out[0][i] = in[0][i];
      out[1][i] = in[1][i];
      continue;
    }

    gen.Process(out0, out1);
  
    out[0][i] = out0;
    out[1][i] = out1;
  }
}

///////////////////////////////////////////////////////////////
// Allocate buffer in SDRAM ////////////////////////////////// 
static const uint32_t kBufferLengthSec = 7;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* sdram_buf[2] = { buf0, buf1 };

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin/Users/vlad/Desktop/clock.h /Users/vlad/Desktop/clock.cpp
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = kPPQN * synthux::kBPMMin / kSecPerMin;

void OnClockTick() {
  if (trig.Tick() && ptn.Tick()) arp.Tick();
}

///////////////////////////////////////////////////////////////
//Arpeggiator callbacks ///////////////////////////////////////
void OnArpTrig(uint8_t slice_num, uint8_t vel) {
  gen.Activate(slice_num);
};

///////////////////////////////////////////////////////////////
///////////////////////// SETUP ///////////////////////////////
void setup() {
  // SETUP DAISY
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sample_rate = DAISY.AudioSampleRate();
  auto buffer_size = DAISY.AudioBlockSize();

  // INIT TOUCH SENSOR
  touch.Init();
  touch.SetOnTouch(OnTouch);
  touch.SetOnRelease(OnRelease);

  // INIT SWITCHES
  pinMode(arp_switch_a, INPUT_PULLUP);
  pinMode(arp_switch_b, INPUT_PULLUP);
  pinMode(reverse_switch, INPUT_PULLUP);
  // pinMode(switch_2_b, INPUT_PULLUP);

  buf.Init(sdram_buf, kBufferLenghtSamples);
  gen.Init(&buf);
  arp.SetOnTrigger(OnArpTrig);

  clk.Init(sample_rate, buffer_size);
  clk.SetOnTick(OnClockTick);
  #ifdef EXTERNAL_SYNC
  pinMode(clk_pin, INPUT);
  #endif

  // BEGIN CALLBACK
  DAISY.begin(AudioCallback);
}

void loop() {
  //PROCESS TOUCH SENSOR
  touch.Process();

  //Recording
  auto new_is_recording = touch.IsTouched(0);
  if (is_recording && !new_is_recording) {
    auto pos = gen.MakeSlices();
    for (auto i = 0; i < kPadsUsed; i++) {
      position_mem[i].Init(static_cast<float>(pos[i]) / static_cast<float>(buf.Length()));
    }
  }
  is_recording = new_is_recording;
  buf.SetRecording(is_recording);

  //Position
  auto position_val = position_knob.Process();
  for (auto i = 0; i < kPadsUsed; i++) {
    auto is_active = touch.IsTouched(i + kLowPad);
    position_mem[i].SetActive(is_active, position_val);
    if (is_active) gen.SetPosition(i, position_mem[i].Process(position_val));
  }
  
  //Pattern
  ptn.SetOnsets(pattern_knob.Process());

  //Envelope
  gen.SetShape(shape_fader.Process());

  // Tempo
  auto tempo = speed_knob.Process();
  clk.SetTempo(tempo);

  //Sync
  #ifdef EXTERNAL_SYNC
  clk.Process(!digitalRead(clk_pin));
  #endif

  //Pitch
  auto pitch_val = pitch_fader.Process();
  if (pitch_val < 0.45 || pitch_val > 0.55) {
      gen.SetSpeed(pitch_val);
  }
  else {
    gen.SetSpeed(0.5);
  }

  //Reverse
  bool is_reverse = digitalRead(reverse_switch);
  gen.SetReverse(is_reverse);

  //Arp mode
  bool is_forward = !digitalRead(arp_switch_b);
  bool is_backward = !digitalRead(arp_switch_a);
  if (is_backward) {
    arp.SetDirection(synthux::ArpDirection::rev);
  } 
  else {
    arp.SetDirection(synthux::ArpDirection::fwd);
  }
  bool as_played = !(is_forward || is_backward);
  arp.SetAsPlayed(as_played);

  delay(4);
}
