// SYNTHUX ACADEMY /////////////////////////////////////////
// ARPEGGIATED SYNTH ///////////////////////////////////////
// CPP Impl

#include "daisy_seed.h"
#include "daisysp.h"

#include "arp.h"
#include "clk.h"
#include "scale.h"
#include "../simple-daisy-touch.h"
#include "vox.h"

using namespace daisy;
using namespace daisysp;

using namespace synthux;
using namespace simpletouch;

////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS //////////////////////////

static const uint8_t kAnalogResolution = 7; // 7bits => 0..127
static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;
static const uint16_t kFirstNotePad = 3;
static const uint16_t kLatchPad = 2;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////

enum AdcChannel {
  speed_knob = 0,        // S30
  length_knob,           // S32
  direction_random_knob, // S33
  NUM_ADC_CHANNELS
};

// Comment this if you're not
// planning using external sync
//#define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const Pin clk_pin = Digital::S32;
#endif

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////

static Scale scale;
static simpletouch::Touch touch;
static Arp<kNotesCount, kPPQN> arp;
static Clock<kPPQN> clck;
static Vox vox;

////////////////////////////////////////////////////////////
////////////////////////// STATE ///////////////////////////

std::array<bool, kNotesCount> hold;
bool latch = false;

////////////////////////////////////////////////////////////
///////////////////// CALLBACKS ////////////////////////////
void OnPadTouch(uint16_t pad) {
  // Latch
  if (pad == kLatchPad) {
    latch = !latch;
    if (!latch) {
      // Drop all latched notes except ones being touched
      for (unsigned i = 0; i < hold.size(); i++) {
        if (hold[i] && !touch.IsTouched(i + kFirstNotePad)) {
          arp.NoteOff(i);
        }
      }
      // Reset latch memory
      hold.fill(false);
    }
    return;
  }

  // Notes
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount)
    return;
  auto note_num = pad - kFirstNotePad;
  if (latch && hold[note_num]) {
    arp.NoteOff(note_num);
    hold[note_num] = false;
  } else {
    arp.NoteOn(note_num, 127);
    hold[note_num] = true;
  }
}
void OnPadRelease(uint16_t pad) {
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount)
    return;
  auto note_num = pad - kFirstNotePad;
  if (!latch) {
    arp.NoteOff(note_num);
    hold[note_num] = false;
  }
}
void OnArpNoteOn(uint8_t num, uint8_t vel) {
  vox.NoteOn(scale.FreqAt(num), vel / 127.f);
}
void OnArpNoteOff(uint8_t num) { vox.NoteOff(); }
void OnClockTick() { arp.Trigger(); }

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  clck.Tick();
  for (size_t i = 0; i < size; i++) {
    out[0][i] = out[1][i] = vox.Process();
  }
}

int main(void) {
  DaisySeed hw;
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

  float sample_rate = hw.AudioSampleRate();
  float buffer_size = hw.AudioBlockSize();

  clck.Init(sample_rate, buffer_size);
  clck.SetOnTick(OnClockTick);
#ifdef EXTERNAL_SYNC
  GPIO clk_input;
  clk_input.Init(clk_pin, GPIO::Mode::INPUT);
#endif
  clck.Run();

  touch.Init(hw);
  touch.SetOnTouch(OnPadTouch);
  touch.SetOnRelease(OnPadRelease);

  arp.SetOnNoteOn(OnArpNoteOn);
  arp.SetOnNoteOff(OnArpNoteOff);

  vox.Init(sample_rate);

  // Configure input
  GPIO mode_switch, scale_switch_a, scale_switch_b;
  mode_switch.Init(Digital::S07, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  scale_switch_a.Init(Digital::S09, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);
  scale_switch_b.Init(Digital::S10, GPIO::Mode::INPUT, GPIO::Pull::PULLUP);

  // Create an ADC configuration
  AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
  adcConfig[speed_knob].InitSingle(Analog::S31);
  adcConfig[length_knob].InitSingle(Analog::S32);
  adcConfig[direction_random_knob].InitSingle(Analog::S33);

  // Initialize the adc with the config we just made
  hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);
  hw.adc.Start();

  hw.SetLed(false);

  hw.StartAudio(AudioCallback);

  // Enable Logging, and set up the USB connection.
  // Set to true if you want logging, and the program, to wait for a Serial
  // Monitor
  hw.StartLog(false);
  hw.PrintLine("Config complete !!!");

  ///////////////////////////////////////////////////////////////
  ////////////////////////// LOOP ///////////////////////////////

  while (1) {
    auto tempo = hw.adc.GetFloat(speed_knob);
    clck.SetTempo(tempo);
#ifdef EXTERNAL_SYNC
    clck.Process(!clk_input.Read());
#endif

    hw.SetLed(latch);

    touch.Process();

    auto arp_length = hw.adc.GetFloat(length_knob);
    auto arp_rand_dir = hw.adc.GetFloat(direction_random_knob);
    auto arp_direction =
        arp_rand_dir < .5f ? ArpDirection::fwd : ArpDirection::rev;
    auto arp_random =
        arp_rand_dir < .5f ? 2.f * arp_rand_dir : 2.f * (1.f - arp_rand_dir);
    arp.SetDirection(arp_direction);
    arp.SetRandChance(arp_random);
    arp.SetAsPlayed(mode_switch.Read());
    arp.SetNoteLength(arp_length);

    auto scale_a_val = static_cast<uint8_t>(!scale_switch_a.Read());
    auto scale_b_val = static_cast<uint8_t>(!scale_switch_b.Read());
    scale.SetScaleIndex(scale_a_val + scale_b_val);

    System::Delay(4);
  }
}
