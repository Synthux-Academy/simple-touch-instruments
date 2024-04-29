// SYNTHUX ACADEMY /////////////////////////////////////////
// TRIPLE LOOPER ///////////////////////////////////////////
#include "simple-daisy-touch.h"
#include "detector.h"
#include "looper.h"
#include "aknob.h"
#include "mvalue.h"
#include "hann.h"

using namespace synthux;

////////////////////////////////////////////////////////////
/////////////////// SDRAM BUFFER /////////////////////////// 
static const uint32_t kBufferLengthSec = 60;
static const uint32_t kSampleRate = 48000;
static const size_t kBufferLenghtSamples = kBufferLengthSec * kSampleRate;
static float DSY_SDRAM_BSS buf0[kBufferLenghtSamples];
static float DSY_SDRAM_BSS buf1[kBufferLenghtSamples];
static float* raw_buf[2] = { buf0, buf1 };

///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static Detector detector;
static Buffer buffer;

static const int kLayerCount = 3;
static const int kWindowSlope = 192;
static synthux::Looper<kWindowSlope> layers[kLayerCount];

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

static std::array<AKnob<>, kLayerCount> mix_knobs = { AKnob(A(S32)), AKnob(A(S33)), AKnob(A(S34)) };
static AKnob speed_knob(A(S30));
static AKnob release_knob(A(S35));

static AKnob start_knob(A(S36));
static AKnob length_knob(A(S37));

static MValue m_start[kLayerCount];
static MValue m_length[kLayerCount];
static MValue m_speed[kLayerCount];
static MValue m_release[kLayerCount];
static MValue m_volume[kLayerCount];
static MValue m_pan[kLayerCount];
static MValue m_in_level;
static MValue m_in_thres;

static int speed_mode_switch = D(S07);

static simpletouch::Touch touch;

uint8_t layer_pads[kLayerCount] = { 3, 5, 7 };

bool monitor_on = false;
bool modifier_is_on = false;

void onTouch(uint16_t pad) {
  auto stop_record = false;
  auto reset = false;
  if (detector.IsArmed()) {
    // Pad 0 works as a switch toggling record on/off. 
    if (pad == 0) stop_record = true;
    else
      // If any of the layer pads is touched while recording 
      // is on, the recording should be stopped.
      for (auto p: layer_pads) if (pad == p) stop_record = true;
  }
  else if (pad == 0) {
    if (modifier_is_on) {
      stop_record = true;
      reset = true;
      buffer.Clear();
    }
    else {
      detector.SetArmed(true);
    }
  }
  
  if (pad == 2) {
    using SM = synthux::LooperSpeedMode;
    for (auto i = 0; i < kLayerCount; i++) {
      if (touch.IsTouched(layer_pads[i])) {
        auto mode = layers[i].SpeedMode() == SM::delta ? SM::increment : SM::delta;
        layers[i].SetSpeedMode(mode);
      }
    }
  }
  
  // After stopping the record the loop length setting
  // should be invalidated and recalculated for all layers
  if (stop_record) {
    detector.SetArmed(false);
    for (auto& l: layers) {
      if (reset) l.Stop();
      else l.InvalidateLength();
    }
  }

  if (pad == 11) monitor_on = !monitor_on;
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
float pre_out[2];
float layer_out[2];
float mix_volume[kLayerCount][2];
float bus[2];
void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    detector.Process(in[0][i], in[1][i], pre_out[0], pre_out[1]);
    buffer.SetRecording(detector.IsOpen());

    if (buffer.IsRecording()) {
      buffer.Write(pre_out[0], pre_out[1], bus[0], bus[1]);
    }
    else if (monitor_on) {
        bus[0] = in[0][i] * m_in_level.Value();
        bus[1] = in[1][i] * m_in_level.Value();
    }
    else {
      bus[0] = 0;
      bus[1] = 0;
    }
    
    for (auto i = 0; i < kLayerCount; i++) {
      if (layers[i].IsPlaying()) { 
        layers[i].Process(layer_out[0], layer_out[1]);
        bus[0] += layer_out[0] * mix_volume[i][0];
        bus[1] += layer_out[1] * mix_volume[i][1];
      }
    }

    out[0][i] = SoftClip(bus[0]);
    out[1][i] = SoftClip(bus[1]);
  }
}

///////////////////////////////////////////////////////////////
///////////////////////// SETUP ///////////////////////////////
void setup() {
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  float sample_rate = DAISY.get_samplerate();
  Serial.begin(9600);

  touch.Init();
  touch.SetOnTouch(onTouch);

  buffer.Init(raw_buf, kBufferLenghtSamples);

  for (auto& t: layers) t.Init(&buffer, sample_rate);

  for (auto i = 0; i < kLayerCount; i++) {
    m_release[i].Init(1.0f);
    m_speed[i].Init(0.5f);
    m_pan[i].Init(0.5f);
    m_volume[i].Init(1.f);
  }

  m_in_level.Init(1.0);
  m_in_thres.Init(0.0056); //~ -45dB

  pinMode(speed_mode_switch, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  DAISY.begin(AudioCallback);
}

bool layer_on = false;
size_t led_counter = 0;
bool led_on = false;
///////////////////////////////////////////////////////////////
///////////////////////// LOOP ////////////////////////////////
void loop() {
  // Use Hann curve keeping the range 0...1, 
  // so it's easier to operate near the middle and extremes
  auto raw_loop_speed = speed_knob.Process();
  auto loop_speed = Hann<kWindowSlope>::fmap_symmetric(raw_loop_speed);
  auto loop_start = start_knob.Process();
  auto loop_length = fmap(length_knob.Process(), 0.f, 1.f, Mapping::EXP);
  auto release = release_knob.Process();

  touch.Process();

  layer_on = false;
  for (auto i = 0; i < kLayerCount; i++) {
    auto mix = mix_knobs[i].Process();

    // Start layer playback
    layer_on = touch.IsTouched(layer_pads[i]);
    auto& l = layers[i];
    l.SetGateOpen(layer_on);

    // Assign per-layer knobs to touched track  
    m_start[i].SetActive(layer_on, loop_start);
    m_length[i].SetActive(layer_on, loop_length);
    m_speed[i].SetActive(layer_on, loop_speed);
    m_release[i].SetActive(layer_on, release);
    m_pan[i].SetActive(layer_on, mix);
    m_volume[i].SetActive(!layer_on, mix);

    // Set per-layer parameters
    if (layer_on) {
      l.SetSpeed(m_speed[i].Process(loop_speed));
      l.SetReverse(raw_loop_speed < .5f);
      l.SetRelease(m_release[i].Process(release));

      auto start = m_start[i].Process(loop_start);
      if (m_start[i].IsChanging()) l.SetStart(start);
      
      auto length = m_length[i].Process(loop_length);
      if (m_length[i].IsChanging()) l.SetLength(length);
      
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

  modifier_is_on = touch.IsTouched(10);

  // If no layer pad is touched the loop start
  // knob controls input level and if pad 10 ("TO")
  // is touched - input treshold
  if (!layer_on) {
    auto in_value = start_knob.Process();
    m_in_thres.SetActive(modifier_is_on, in_value);
    m_in_level.SetActive(!modifier_is_on, in_value);
    detector.SetTreshold(m_in_thres.Process(in_value));
    buffer.SetLevel(m_in_level.Process(fmap(in_value, .001f, .89f, Mapping::EXP))); // ~ -60...-1 dB
  }

  // Indicate recording state
  if (detector.IsOpen()) {
    led_on = true;
  }
  else if (detector.IsArmed()) {
    if (++ led_counter == 50) {
      led_on = !led_on;
      led_counter = 0;
    }
  }
  else {
    led_on = false;
  }
  digitalWrite(LED_BUILTIN, led_on);

  delay(4);
}
