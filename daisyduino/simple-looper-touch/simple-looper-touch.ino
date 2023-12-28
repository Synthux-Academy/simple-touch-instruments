// SYNTHUX ACADEMY /////////////////////////////////////////
// TRIPLE LOOPER ///////////////////////////////////////////
#include "simple-daisy-touch.h"
#include "looper.h"
#include "aknob.h"
#include "mvalue.h"

using namespace synthux;

static const int kTracksCount = 3;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////
static std::array<AKnob<>, kTracksCount> mix_knobs = { AKnob(A(S32)), AKnob(A(S33)), AKnob(A(S34)) };
static AKnob speed_knob(A(S30));
static AKnob release_knob(A(S35));

static AKnob start_knob(A(S36));
static AKnob length_knob(A(S37));

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
void AudioCallback(float **in, float **out, size_t size) {
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
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();

  Serial.begin(9600);

  touch.Init();

  buffer.Init(buf, kBufferLenghtSamples);

  for (auto& t: tracks) t.Init(&buffer);

  for (auto i = 0; i < kTracksCount; i++) {
    m_length[i].Init(0.3f);
    m_release[i].Init(1.0f);
    m_speed[i].Init(0.75f);
    m_pan[i].Init(0.5f);
    m_volume[i].Init(1.f);
  }

  DAISY.begin(AudioCallback);
}

uint8_t t_act_idx = 0;
uint8_t track_pads[kTracksCount] = { 3, 5, 7 };
bool is_reverse_touched = false;

///////////////////////////////////////////////////////////////
///////////////////////// LOOP ////////////////////////////////
void loop() {
  auto loop_speed = speed_knob.Process();
  auto loop_start = start_knob.Process();
  auto loop_length = fmap(length_knob.Process(), 0.f, 1.f, Mapping::EXP);
  auto release = release_knob.Process();

  touch.Process();

  for (auto i = 0; i < kTracksCount; i++) {
    auto mix = mix_knobs[i].Process();

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

  delay(4);
}
