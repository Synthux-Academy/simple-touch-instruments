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

////////////////////////////////////////////////////////////
///////////////////// KNOBS & SWITCHES /////////////////////
static synthux::AKnob speed_knob(A(S30));
static synthux::AKnob pattern_knob(A(S32));
static synthux::AKnob position_knob(A(S33));
// static const int knob_d = A(S34);
// static const int knob_e = A(S35);

static synthux::AKnob shape_fader(A(S36));
static synthux::AKnob pitch_fader(A(S37));

static std::array<synthux::MKnob, 7> position_mem;

static const int reverse_switch = D(S07);
// static const int switch_1_b = D(S08);
static const int arp_switch_a = D(S09);
static const int arp_switch_b = D(S10);

////////////////////////////////////////////////////////////
////////////////////////// TOUCH  //////////////////////////
static synthux::simpletouch::Touch touch;
static constexpr int kPadsUsed = 7;
static constexpr int kLowPad = 3;
static constexpr int kHighPad = kLowPad + kPadsUsed - 1; 
static constexpr int kRecordPad = 0;

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static synthux::Buffer buf;
static synthux::Generator<kPadsUsed> gen;
static synthux::CPattern ptn;
static synthux::Trigger trig;
static synthux::TrigArp<kPadsUsed> arp;
static Metro mtr;

///////////////////////////////////////////////////////////////
///////////////////////// CALLBACKS ///////////////////////////
void OnTouch(uint16_t pad) {
  if (pad >= kLowPad && pad <= kHighPad) {
    arp.SetTrigger(pad - kLowPad, 127);
  }
}

void OnRelease(uint16_t pad) {
  if (pad >= kLowPad && pad <= kHighPad) {
    arp.RemoveTrigger(pad - kLowPad);
  }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
bool is_recording = false;

void AudioCallback(float **in, float **out, size_t size) {
  auto out0 = 0.f;
  auto out1 = 0.f;

  for (size_t i = 0; i < size; i++) {
    if (mtr.Process() && trig.Tick() && ptn.Tick()) {
      arp.Tick();
    }

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
static const uint32_t kBufferLengthSec = 5;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* sdram_buf[2] = { buf0, buf1 };

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = synthux::Trigger::kPPQN * kBPMRange / kSecPerMin;

///////////////////////////////////////////////////////////////
//Arpeggiator callbacks ////////.......////////////////////////
void OnArpTrig(uint8_t slice_num, uint8_t vel) {
  gen.Activate(slice_num);
};

///////////////////////////////////////////////////////////////
///////////////////////// SETUP ///////////////////////////////
void setup() {
  // SETUP DAISY
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sample_rate = DAISY.get_samplerate();

  Serial.begin(9600);

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
  mtr.Init(96, sample_rate); //96Hz = 48ppqn @ 120bpm 

  // BEGIN CALLBACK
  DAISY.begin(AudioCallback);
}

void loop() {
  //PROCESS TOUCH SENSOR
  touch.Process();

  //Position
  auto position_val = position_knob.Process();
  for (auto i = 0; i < kPadsUsed; i++) {
    auto is_active = touch.IsTouched(i + kLowPad);
    position_mem[i].SetActive(is_active, position_val);
    if (is_active) gen.SetPosition(i, position_mem[i].Process(position_val));
  }
  
  ptn.SetOnsets(pattern_knob.Process());
  gen.SetShape(shape_fader.Process());

  // Speed
  auto speed = speed_knob.Process();
  auto freq = kMinFreq + kFreqRange * speed;
  mtr.SetFreq(freq);

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
