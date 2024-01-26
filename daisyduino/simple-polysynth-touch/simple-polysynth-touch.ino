#include "simple-daisy-touch.h"
#include "vox.h"
#include "scale.h"
#include "driver.h"
#include "aknob.h"
#include <array>

using namespace synthux;
using namespace simpletouch;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////
static AKnob some_knob(A0);


///////////////////////////////////////////////////////////////
///////////////////////// MODULES /////////////////////////////
static constexpr uint8_t kVoxCount = 4;

static std::array<Vox, kVoxCount> vox;
static Driver<kVoxCount> driver;
static Scale scale;
static Touch touch;
static Oscillator lfo;

static constexpr uint8_t kNone = 0xff;
std::array<uint8_t, kVoxCount> notes;
std::array<uint8_t, kVoxCount> order;
uint8_t touchedPadsCount = 0;

void onTouch(uint16_t pad) {
  auto vox_index = driver.onNoteOn(pad);
  auto note = driver.noteAt(vox_index);
  vox[vox_index].SetFreq(scale.FreqAt(note));
} 

void onRelease(uint16_t pad) {
  driver.onNoteOff(pad);
}

void releaseAt(uint8_t index) {
  auto note = order[index];
  uint8_t i;
  for (i = 0; i < kVoxCount; i++) {
    if (notes[i] == note) notes[i] = kNone;
  }
  for (i = index; i < kVoxCount - 1; i++) {
    order[i] = order[i+1];
  }
  touchedPadsCount--;
}

float output;
void AudioCallback(float **in, float **out, size_t size) {
  for (size_t i = 0; i < size; i++) {
    output = 0;
    for (auto k = 0; k < kVoxCount; k++) {
      output += vox[k].Process(driver.gateAt(k));
    }
    out[0][i] = out[1][i] = output * (0.15f - lfo.Process());
  }
}

void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  auto sampleRate = DAISY.get_samplerate();

  lfo.Init(sampleRate);
  lfo.SetFreq(2);
  lfo.SetWaveform(Oscillator::WAVE_TRI);
  lfo.SetAmp(0.1);

  some_knob.Init();

  for (auto& v: vox) v.Init(sampleRate);

  touch.Init();
  touch.SetOnTouch(onTouch);
  touch.SetOnRelease(onRelease);

  DAISY.begin(AudioCallback);
}

void loop() {
  touch.Process(); 

  auto knob_value = some_knob.Process();

  delay(4);
}
