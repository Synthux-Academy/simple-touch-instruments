
#include "simple-daisy-touch.h"
#include "looper.h"
#include "aknob.h"
#include "mknob.h"

using namespace synthux;

static const int kTracksCount = 3;

// Setup pins
static std::array<AKnob<>, kTracksCount> volume_knobs = { AKnob(A(S32)), AKnob(A(S33)), AKnob(A(S34)) };
static AKnob speed_knob(A(S30));
static AKnob release_knob(A(S35));

static AKnob start_knob(A(S36));
static AKnob length_knob(A(S37));

static MKnob m_start[kTracksCount];
static MKnob m_length[kTracksCount];
static MKnob m_speed[kTracksCount];
static MKnob m_release[kTracksCount];

static simpletouch::Touch touch;

// Allocate buffer in SDRAM 
static const uint32_t kBufferLengthSec = 7;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* buf[2] = { buf0, buf1 };

static Buffer buffer;
static synthux::Looper<> tracks[kTracksCount];

float tracks_sum[2];
float track_out[2];
float mix_volume[kTracksCount];
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
        tracks_sum[0] += track_out[0] * mix_volume[i];
        tracks_sum[1] += track_out[1] * mix_volume[i];
      }
    }
    out[0][i] = tracks_sum[0];
    out[1][i] = tracks_sum[1];
  }
}

void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();

  //Serial.begin(9600);

  touch.Init();

  buffer.Init(buf, kBufferLenghtSamples);

  for (auto& t: tracks) t.Init(&buffer);

  for (auto i = 0; i < kTracksCount; i++) {
    m_length[i].Init(0.3f);
    m_release[i].Init(1.0f);
  }

  DAISY.begin(AudioCallback);
}

uint8_t t_act_idx = 0;
uint8_t track_pads[kTracksCount] = { 3, 5, 7 };
bool is_reverse_touched = false;

void loop() {
  auto raw_loop_speed = speed_knob.Process();
  auto loop_speed = 0.f;
  if (raw_loop_speed < 0.475f || raw_loop_speed > 0.525f) {
    loop_speed = fmap(raw_loop_speed, -1.f, 1.f);
  }
  auto loop_start = start_knob.Process();
  auto loop_length = fmap(length_knob.Process(), 0.f, 1.f, Mapping::EXP);
  auto release = release_knob.Process();

  touch.Process();

  t_act_idx = UINT8_MAX;
  for (auto i = 0; i < kTracksCount; i++) {
    auto is_on = touch.IsTouched(track_pads[i]);
    if (is_on) t_act_idx = i;
    tracks[i].SetGateOpen(is_on);
    
    mix_volume[i] = fmap(volume_knobs[i].Process(), 0.f, 1.f, Mapping::EXP);

    // Assign per-track knobs to touched track  
    auto is_active = (i == t_act_idx);
    m_start[i].SetActive(is_active, loop_start);
    m_length[i].SetActive(is_active, loop_length);
    m_speed[i].SetActive(is_active, loop_speed);
    m_release[i].SetActive(is_active, release);
  }
  
  // Set per-track parameters
  if (t_act_idx < UINT8_MAX) {
    auto& t = tracks[t_act_idx];
    t.SetSpeed(1.f + m_speed[t_act_idx].Process(loop_speed));
    t.SetLoop(m_start[t_act_idx].Process(loop_start), m_length[t_act_idx].Process(loop_length));
    t.SetRelease(m_release[t_act_idx].Process(release));
    if (touch.IsTouched(2) && !is_reverse_touched) {
      t.ToggleDirection();
    }
    is_reverse_touched = touch.IsTouched(2);
  }

  // Toggle record
  auto record_on = touch.IsTouched(0);
  buffer.SetRecording(record_on);

  delay(4);
}
