// SYNTHUX ACADEMY /////////////////////////////////////////
// TRIPLE LOOPER ///////////////////////////////////////////

#include "daisy_seed.h"
#include "daisysp.h"

#include "../simple-daisy-touch.h"
#include "looper.h"
#include "mvalue.h"

using namespace daisy;
using namespace daisysp;

using namespace synthux;
using namespace simpletouch;

static const int kTracksCount = 3;
uint8_t track_pads[kTracksCount] = { 3, 5, 7 };
bool is_reverse_touched = false;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////
static std::array<int, kTracksCount> mix_knobs = { AdcChannel::S32, AdcChannel::S33, AdcChannel::S34 };

static const int speed_knob = AdcChannel::S30;
static const int release_knob = AdcChannel::S35;
static const int start_knob = AdcChannel::S36;
static const int length_knob = AdcChannel::S37;

static MValue m_start[kTracksCount];
static MValue m_length[kTracksCount];
static MValue m_speed[kTracksCount];
static MValue m_release[kTracksCount];
static MValue m_volume[kTracksCount];
static MValue m_pan[kTracksCount];

static simpletouch::Touch touch;

////////////////////////////////////////////////////////////
/////////////////// SDRAM BUFFER /////////////////////////// 
static const uint32_t kBufferLengthSec = 15;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* buf[2] = { buf0, buf1 };

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
DaisySeed hw;
static Buffer buffer;
static synthux::Looper<> tracks[kTracksCount];

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float tracks_sum[2];
float track_out[2];
float mix_volume[kTracksCount][2];
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
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
int main(void) {
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  //Serial.begin(9600); ??Huh

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

  hw.StartAudio(AudioCallback);

  while(1){
    auto loop_speed = touch.GetAdcValue(speed_knob);
    auto loop_start = touch.GetAdcValue(start_knob);
    auto length = touch.GetAdcValue(release_knob);
    auto loop_length = fmap(length, 0.f, 1.f, Mapping::EXP);
    auto release = touch.GetAdcValue(release_knob);

    touch.Process();

    for (auto i = 0; i < kTracksCount; i++) {
      auto mix = touch.GetAdcValue(mix_knobs[i]);

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

    hw.DelayMs(4);
  }
}