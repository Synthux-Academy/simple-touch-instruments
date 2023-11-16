
#include "simple-daisy.h"
#include "looper.h"
#include "aknob.h"
#include "memknob.h"
#include "term.h"
#include "mod.h"

static const int kTracksCount = 3;

// Setup pins
static synthux::AKnob start_knob(A(S33));
static synthux::AKnob length_knob(A(S30));
static synthux::AKnob rand_knob(A(S34));
static synthux::AKnob speed_knob(A(S37));
static synthux::AKnob release_knob(A(S36));

static synthux::MemKnob mem_start[kTracksCount];
static synthux::MemKnob mem_length[kTracksCount];
static synthux::MemKnob mem_speed[kTracksCount];
static synthux::MemKnob mem_release[kTracksCount];
static synthux::MemKnob mem_rand[kTracksCount];

static synthux::Terminal term;

// Allocate buffer in SDRAM 
static const uint32_t kBufferLengthSec = 5;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* buf[2] = { buf0, buf1 };

static synthux::Buffer buffer;
static synthux::Modulator mod;
static synthux::Looper<> tracks[kTracksCount];

float attenuation = 0.9f;
float t_sum[2];
float t_out[2];
void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    mod.Process();
    buffer.Write(in[0][i], in[1][i]);
    if (buffer.IsRecording()) {
      out[0][i] = in[0][i] * attenuation;
      out[1][i] = in[1][i] * attenuation;
      continue;
    };
    
    t_sum[0] = 0;
    t_sum[1] = 0;
    for (auto& t: tracks) {
      if (t.IsPlaying()) { 
        t.Process(t_out[0], t_out[1]);
        t_sum[0] += t_out[0];
        t_sum[1] += t_out[1];
      }
    }
    out[0][i] = t_sum[0] * attenuation;
    out[1][i] = t_sum[1] * attenuation;
  }
}

void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();

  Serial.begin(9600);

  term.Init();

  buffer.Init(buf, kBufferLenghtSamples);
  mod.Init(sample_rate);

  for (auto& t: tracks) t.Init(&buffer, &mod);

  start_knob.Init();
  length_knob.Init();
  rand_knob.Init();
  speed_knob.Init();

  for (auto i = 0; i < kTracksCount; i++) {
    mem_length[i].Init(0.3f);
    mem_release[i].Init(1.0f);
  }

  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

uint8_t t_act_idx = 0;
uint8_t idx[kTracksCount] = { 3, 4, 6 };
bool is_reverse_touched = false;

void loop() {
  auto raw_loop_speed = speed_knob.Process();
  auto loop_speed = 0.f;
  if (raw_loop_speed < 0.475f || raw_loop_speed > 0.525f) {
    loop_speed = fmap(raw_loop_speed, -1.f, 1.f);
  }
  auto loop_start = start_knob.Process();
  auto loop_length = fmap(length_knob.Process(), 0.f, 1.f, Mapping::EXP);
  auto loop_rand = rand_knob.Process();
  auto release = release_knob.Process();

  term.Process();

  t_act_idx = UINT8_MAX;

  for (auto i = 0; i < kTracksCount; i++) {
    auto pin = idx[i];
    auto is_on = term.IsOn(pin);
    if (is_on) t_act_idx = i;
    tracks[i].SetGateOpen(is_on);
    
    auto is_active = (i == t_act_idx);
    mem_start[i].SetActive(is_active, loop_start);
    mem_length[i].SetActive(is_active, loop_length);
    mem_speed[i].SetActive(is_active, loop_speed);
    mem_rand[i].SetActive(is_active, loop_rand);
    mem_release[i].SetActive(is_active, release);
  }

  
  if (t_act_idx < UINT8_MAX) {
    auto& t = tracks[t_act_idx];
    t.SetSpeed(1.f + mem_speed[t_act_idx].Process(loop_speed));
    t.SetLoop(mem_start[t_act_idx].Process(loop_start), mem_length[t_act_idx].Process(loop_length));
    t.SetRandAmount(mem_rand[t_act_idx].Process(loop_rand));
    t.SetRelease(mem_release[t_act_idx].Process(release));
    if (term.IsTouched(2) && !is_reverse_touched) {
      t.ToggleDirection();
    }
    is_reverse_touched = term.IsTouched(2);
  }

  // Toggle record
  auto record_on = term.IsTouched(0);
  buffer.SetRecording(record_on);

  // auto mod_freq = fmap(analogRead(loop_speed_pin) / kKnobMax, 5.f, 100.0f);
  // mod.SetFreq(mod_freq);

  // // Set pitch
  // mod_val = fmap(analogRead(mod_pin) / kKnobMax, 0.f, 1.0f);

  digitalWrite(LED_BUILTIN, term.IsLatched());

  delay(4);
}
