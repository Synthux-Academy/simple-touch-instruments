// SYNTHUX ACADEMY /////////////////////////////////////////
// TRIPLE LOOPER ///////////////////////////////////////////
#include "daisy_seed.h"
#include "daisysp.h"

#include "../simple-daisy-touch.h"
#include "looper.h"
//#include "aknob.h"
#include "mvalue.h"

using namespace daisy;
using namespace daisysp;

using namespace synthux;
using namespace simpletouch;

static const int kTracksCount = 3;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////

enum AdcChannel {
  mix1_knob = 0,  // S32
  mix2_knob, // S33
  mix3_knob, // S34
  speed_knob, // S30
  release_knob, // S35
  start_knob, // S36
  length_knob, // S37
  NUM_ADC_CHANNELS
};

static std::array<int, kTracksCount> mix_knobs = { mix1_knob, mix2_knob, mix3_knob };

//static std::array<AKnob<>, kTracksCount> mix_knobs = { AKnob(A(S32)), AKnob(A(S33)), AKnob(A(S34)) };
//static AKnob speed_knob(A(S30));
//static AKnob release_knob(A(S35));

//static AKnob start_knob(A(S36));
//static AKnob length_knob(A(S37));

static MValue m_start[kTracksCount];
static MValue m_length[kTracksCount];
static MValue m_speed[kTracksCount];
static MValue m_release[kTracksCount];
static MValue m_volume[kTracksCount];
static MValue m_pan[kTracksCount];

static simpletouch::Touch touch;

////////////////////////////////////////////////////////////
/////////////////// SDRAM BUFFER /////////////////////////// 
static const uint32_t kBufferLengthSec = 7;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* buf[2] = { buf0, buf1 };

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static Buffer buffer;
static synthux::Looper<> tracks[kTracksCount];

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float tracks_sum[2];
float track_out[2];
float mix_volume[kTracksCount][2];
void AudioCallback(AudioHandle::InputBuffer in, 
                  AudioHandle::OutputBuffer out,
                   size_t size) {
  for (size_t i = 0; i < size; i++) {
    buffer.Write(in[0][i], in[1][i]);
    if (buffer.IsRecording()) {
      out[0][i] = in[0][i];
      out[1][i] = in[1][i];
      continue;
    };
    
    tracks_sum[0] = 0;
    tracks_sum[1] = 0;
    for (auto i = 0; i < kTracksCount; i++) {
      if (tracks[i].IsPlaying()) { 
        tracks[i].Process(track_out[0], track_out[1]);
        tracks_sum[0] += track_out[0] * mix_volume[i][0];
        tracks_sum[1] += track_out[1] * mix_volume[i][1];
      }
    }
    out[0][i] = tracks_sum[0];
    out[1][i] = tracks_sum[1];
  }
}

///////////////////////////////////////////////////////////////
///////////////////////// SETUP ///////////////////////////////
void setup() {
  DaisySeed hw;
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  touch.Init(hw);

  buffer.Init(buf, kBufferLenghtSamples);

  for (auto& t: tracks) t.Init(&buffer);

  for (auto i = 0; i < kTracksCount; i++) {
    m_length[i].Init(0.3f);
    m_release[i].Init(1.0f);
    m_speed[i].Init(0.75f);
    m_pan[i].Init(0.5f);
    m_volume[i].Init(1.f);
  }

  // Enable Logging, and set up the USB connection.
  // Set to true if you want logging, and the program, to wait for a Serial
  // Monitor
  hw.StartLog(false);
  hw.PrintLine("Config complete !!!");

  uint8_t track_pads[kTracksCount] = { 3, 5, 7 };

  // Create an ADC configuration
  AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
  adcConfig[mix1_knob].InitSingle(Analog::S32);
  adcConfig[mix2_knob].InitSingle(Analog::S33);
  adcConfig[mix3_knob].InitSingle(Analog::S34);
  adcConfig[speed_knob].InitSingle(Analog::S30);
  adcConfig[release_knob].InitSingle(Analog::S35);
  adcConfig[start_knob].InitSingle(Analog::S36);
  adcConfig[length_knob].InitSingle(Analog::S37);

  // Initialize the adc with the config we just made
  hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);
  hw.adc.Start();

  ///////////////////////////////////////////////////////////////
  ///////////////////////// LOOP ////////////////////////////////
  while(1){
    float loop_speed = hw.adc.GetFloat(speed_knob);
    float loop_start = hw.adc.GetFloat(start_knob);
    float loop_length = fmap(hw.adc.GetFloat(length_knob), 
        0.f, 1.f, Mapping::EXP);
    float release = hw.adc.GetFloat(release_knob);

    touch.Process();
    
    for (auto i = 0; i < kTracksCount; i++) {
      auto mix = hw.adc.GetFloat(mix_knobs[i]);

      // Start track playback
      auto is_on = touch.IsTouched(track_pads[i]);
      auto& t = tracks[i];
      t.SetGateOpen(is_on);

      // Assign per-track knobs to touched track  
      m_start[i].SetActive(is_on, loop_start);
      m_length[i].SetActive(is_on, loop_length);
      m_speed[i].SetActive(is_on, loop_speed);
      m_release[i].SetActive(is_on, release);
      m_pan[i].SetActive(is_on, mix);
      m_volume[i].SetActive(!is_on, mix);

      // Set per-track parameters
      if (is_on) {
        t.SetSpeed(m_speed[i].Process(loop_speed));
        t.SetLoop(m_start[i].Process(loop_start), m_length[i].Process(loop_length));
        t.SetRelease(m_release[i].Process(release));
        m_pan[i].Process(mix);
      }
      else {
        m_volume[i].Process(mix);
      }

      auto volume = fmap(m_volume[i].Value(), 0.f, 1.f, Mapping::EXP);
      auto pan = m_pan[i].Value();
      float pan0 = 1.f;
      float pan1 = 1.f;  
      if (pan < 0.47f) pan1 = 2.f * pan;
      else if (pan > 0.53f) pan0 = 2.f * (1.f - pan);
      mix_volume[i][0] = volume * pan0;
      mix_volume[i][1] = volume * pan1;
    }

    // Toggle record
    auto record_on = touch.IsTouched(0);
    buffer.SetRecording(record_on);

     System::Delay(4);
  }
}
