// SYNTHUX ACADEMY /////////////////////////////////////////
// SIMPLE FX BOX ///////////////////////////////////////////

#include "simple-touch-daisy.h"
#include "aknob.h"
#include "echo.h"
#include "softswitch.h"
#include "xfade.h"
#include "mvalue.h"
#include <array>

using namespace synthux;
using namespace infrasonic;

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

static AKnob dly_fb_knob(A(S31));
static AKnob dly_mix_knob(A(S30));
static AKnob dly_mod_speed_knob(A(S32));
static AKnob dly_mod_amount_knob(A(S33));
static AKnob dly_time_fader(A(S36));

static AKnob verb_send_knob(A(S34));
static AKnob verb_mix_drv_level_knob(A(S35));
static AKnob verb_fb_fader(A(S37));

static MValue verb_mix;

static SoftSwitch drv_on;
static SoftSwitch dcm_on;
static SoftSwitch bypass_on;
static SoftSwitch dly_bypass_on;
static SoftSwitch verb_bypass_on;

static XFade drv_fade;
static XFade dcm_fade;
static XFade bypass_fade;
static XFade dly_fade;
static XFade verb_fade;

static const int switch_1_a = D(S07);
static const int switch_1_b = D(S08);
static const int switch_2_a = D(S09);
static const int switch_2_b = D(S10);

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static const uint32_t kBufferLengthSec = 5;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS delay_buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS delay_buf1[kBufferLenghtSamples];
static EchoDelay<kBufferLenghtSamples> dly[2];

static ReverbSc verb;
static Oscillator lfo;
static Overdrive drv[2];
static Decimator dcm[2];

////////////////////////////////////////////////////////////
////////////////////////// TOUCH  //////////////////////////
static synthux::simpletouch::Touch touch;

bool latch = false;
uint8_t drv_mix_index;
uint8_t dcm_mix_index;
std::array<MValue, 3> drv_mix_values;
std::array<MValue, 3> dcm_mix_values;

void OnTouch(uint16_t pad) {
  bool drive = false;
  float drv_level;

  bool crush = false;
  float down_level;
  float crush_level;

  switch (pad) {
    case 0: drive = true; drv_level = .36f; drv_mix_index = 0; break;
    case 3: drive = true; drv_level = .41f; drv_mix_index = 1; break;
    case 4: drive = true; drv_level = .52f; drv_mix_index = 2; break;
    case 5: bypass_on.SetOn(!(latch && bypass_on.IsOn())); break; 
    case 2: crush = true; down_level = 0.f; crush_level = .86f; dcm_mix_index = 0; break;
    case 6: crush = true; down_level = .23f; crush_level = .25f; dcm_mix_index = 1; break;
    case 7: crush = true; down_level = .69f; crush_level = .9f; dcm_mix_index = 2; break;
    case 8: dly_bypass_on.SetOn(!(latch && dly_bypass_on.IsOn())); break;
    case 9: verb_bypass_on.SetOn(!(latch && verb_bypass_on.IsOn())); break;
    default: break;
  }

  if (drive) {
    drv[0].SetDrive(drv_level);
    drv[1].SetDrive(drv_level);
    drv_on.SetOn(!(latch && drv_on.IsOn()));
  }

  if (crush) {
    dcm[0].SetDownsampleFactor(down_level);
    dcm[1].SetDownsampleFactor(down_level);
    dcm[0].SetBitcrushFactor(crush_level);
    dcm[1].SetBitcrushFactor(crush_level);
    dcm_on.SetOn(!(latch && dcm_on.IsOn()));
  }
}

void OnRelease(uint16_t pad) {
  if (!latch) {
    switch (pad) {
      case 0: drv_on.SetOn(false); break;
      case 3: drv_on.SetOn(false); break;
      case 4: drv_on.SetOn(false); break;
      case 5: bypass_on.SetOn(false); break;
      case 2: dcm_on.SetOn(false); break;
      case 6: dcm_on.SetOn(false); break;
      case 7: dcm_on.SetOn(false); break;
      case 8: dly_bypass_on.SetOn(false); break;
      case 9: verb_bypass_on.SetOn(false); break;
    }
  }
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float dly_mix;
float dly_time;
float dly_bypass;
float dly_in[2];

float verb_send;
float verb_bypass;
float verb_in[2];
float verb_out[2];

float drv_mix;
float dcm_mix;

float bus0;
float bus1;

void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    auto t = dly_time + 0.25 * lfo.Process();
    dly[0].SetDelayTime(t);
    dly[1].SetDelayTime(t);

    bus0 = in[0][i];
    bus1 = in[1][i];

    drv_mix = drv_mix_values[drv_mix_index].Value();
    drv_fade.SetStage(drv_on.Process());
    drv_fade.Process(bus0, bus1, drv[0].Process(bus0) * drv_mix, drv[1].Process(bus1) * drv_mix, bus0, bus1);

    dcm_mix = dcm_mix_values[dcm_mix_index].Value();
    dcm_fade.SetStage(dcm_on.Process());
    dcm_fade.Process(bus0, bus1, dcm[0].Process(bus0) * dcm_mix, dcm[1].Process(bus1) * dcm_mix, bus0, bus1);
      
    dly_bypass = dly_bypass_on.Process(true) * dly_mix;
    dly_in[0] = bus0 * dly_bypass;
    dly_in[1] = bus1 * dly_bypass;
    dly_fade.SetStage(dly_bypass);
    dly_fade.Process(bus0, bus1, dly[0].Process(dly_in[0]), dly[1].Process(dly_in[1]), bus0, bus1);

    verb_bypass = verb_bypass_on.Process(true) * verb_mix.Value();
    verb_in[0] = bus0 * verb_bypass * verb_send;
    verb_in[1] = bus1 * verb_bypass * verb_send;
    verb.Process(verb_in[0], verb_in[1], &(verb_out[0]), &(verb_out[1]));
    verb_fade.SetStage(verb_bypass);
    verb_fade.Process(bus0, bus1, verb_out[0], verb_out[1], bus0, bus1);

    out[0][i] = SoftLimit(bus0);
    out[1][i] = SoftLimit(bus1);
  }
}

///////////////////////////////////////////////////////////////
///////////////////// SETUP ///////////////////////////////////
void setup() {
  // SETUP DAISY
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sample_rate = DAISY.get_samplerate();

  // INIT TOUCH SENSOR
  touch.Init();
  touch.SetOnTouch(OnTouch);
  touch.SetOnRelease(OnRelease);

  // INIT SWITCHES
  pinMode(switch_1_a, INPUT_PULLUP);
  pinMode(switch_1_b, INPUT_PULLUP);
  pinMode(switch_2_a, INPUT_PULLUP);
  pinMode(switch_2_b, INPUT_PULLUP);

  drv_on.Init(sample_rate);
  dcm_on.Init(sample_rate);
  bypass_on.Init(sample_rate);
  dly_bypass_on.Init(sample_rate);
  verb_bypass_on.Init(sample_rate);

  drv_mix_values[0].Init(.52f);
  drv_mix_values[1].Init(.28f);
  drv_mix_values[2].Init(.20f);

  dcm_mix_values[0].Init(.8f);
  dcm_mix_values[1].Init(.35f);
  dcm_mix_values[2].Init(.5f);

  drv[0].Init();
  drv[1].Init();
  drv_fade.SetStage(1.f);

  for (auto& d: dcm) {
    d.Init();
    d.SetDownsampleFactor(1.0);
  }
  dcm_fade.SetStage(1.f);

  verb.Init(sample_rate);
  verb.SetLpFreq(8000.f);

  dly[0].Init(sample_rate, delay_buf0);
  dly[0].SetLagTime(.3f);
  dly[0].SetFeedback(.6f);
  
  dly[1].Init(sample_rate, delay_buf1);
  dly[1].SetLagTime(.3f);
  dly[1].SetFeedback(.6f);
  
  lfo.Init(sample_rate);
  lfo.SetWaveform(Oscillator::WAVE_TRI);

  // BEGIN CALLBACK
  DAISY.begin(AudioCallback);
}

bool new_latch;
void loop() {
  //PROCESS TOUCH SENSOR
  touch.Process();

  lfo.SetFreq(fmap(dly_mod_speed_knob.Process(), 1.0, 500.0));
  lfo.SetAmp(fmap(dly_mod_amount_knob.Process(), 0.f, 0.8f));  

  //Process knob values
  dly_mix = dly_mix_knob.Process();
  dly_time = fmap(dly_time_fader.Process(), 0.f, 5.f, Mapping::EXP);
  auto dly_fb = dly_fb_knob.Process() * 1.02;
  dly[0].SetFeedback(dly_fb);
  dly[1].SetFeedback(dly_fb);

  verb.SetFeedback(fmap(verb_fb_fader.Process(), 0.3f, 1.f));
  verb_send = verb_send_knob.Process();
  auto verb_mix_drv_level = verb_mix_drv_level_knob.Process();

  auto notTouched = !touch.hasTouched();
  verb_mix.SetActive(notTouched, verb_mix_drv_level);
  verb_mix.Process(verb_mix_drv_level);

  auto drv_dcm_mix = verb_mix_drv_level * .8f;
  drv_mix_values[0].SetActive(touch.IsTouched(0), drv_dcm_mix);
  drv_mix_values[1].SetActive(touch.IsTouched(3), drv_dcm_mix);
  drv_mix_values[2].SetActive(touch.IsTouched(4), drv_dcm_mix);
  drv_mix_values[0].Process(drv_dcm_mix);
  drv_mix_values[1].Process(drv_dcm_mix);
  drv_mix_values[2].Process(drv_dcm_mix);

  dcm_mix_values[0].SetActive(touch.IsTouched(2), drv_dcm_mix);
  dcm_mix_values[1].SetActive(touch.IsTouched(6), drv_dcm_mix);
  dcm_mix_values[2].SetActive(touch.IsTouched(7), drv_dcm_mix);
  dcm_mix_values[0].Process(drv_dcm_mix);
  dcm_mix_values[1].Process(drv_dcm_mix);
  dcm_mix_values[2].Process(drv_dcm_mix);

  //Process switch values
  //The value of digitalRead is inverted
  new_latch = digitalRead(switch_1_a);
  if (latch && !new_latch) {
    dly_bypass_on.SetOn(false);
    verb_bypass_on.SetOn(false);
    bypass_on.SetOn(false);
    drv_on.SetOn(false);
    dcm_on.SetOn(false);
  }
  latch = new_latch;

  delay(4);
}
