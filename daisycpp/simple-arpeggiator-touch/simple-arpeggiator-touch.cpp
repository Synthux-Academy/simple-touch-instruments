#include "daisy_seed.h"
#include "daisysp.h"

#include "simple-daisy-touch.h"
#include "clk.h"
#include "vox.h"
#include "arp.h"
#include "scale.h"

using namespace daisy;
using namespace daisysp;


using namespace synthux;
using namespace simpletouch;

////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS //////////////////////////

static const uint8_t kAnalogResolution = 7; //7bits => 0..127
static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;
static const uint16_t kFirstNotePad = 3;
static const uint16_t kLatchPad = 2;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////

GPIO mode_switch, scale_switch_a, scale_switch_b;

enum AdcChannel {
   speed_knob= 0,
   length_knob,
   direction_random_knob,
   NUM_ADC_CHANNELS
};

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////

static Scale scale;
static simpletouch::Touch touch;
static Arp<kNotesCount, kPPQN> arp;
static Metro metro;
static Vox vox;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = kPPQN * kBPMRange / kSecPerMin;

////////////////////////////////////////////////////////////
///////////////////// CALLBACKS ////////////////////////////

void OnScaleSelect(uint8_t index) { scale.SetScaleIndex(index); }
void OnTerminalNoteOn(uint8_t num, uint8_t vel) { arp.NoteOn(num, vel); }
void OnTerminalNoteOff(uint8_t num) { arp.NoteOff(num); }
void OnArpNoteOn(uint8_t num, uint8_t vel) { vox.NoteOn(scale.FreqAt(num), vel / 127.f); }
void OnArpNoteOff(uint8_t num) { vox.NoteOff(); }

DaisySeed hw;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  for (size_t i = 0; i < size; i++) {
    if (metro.Process()) arp.Trigger();
		out[0][i] = out[1][i] = vox.Process();
  }
}

int main(void) 
{
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

 	float sample_rate = hw.AudioSampleRate();
	vox.Init(sample_rate);

  metro.Init(48, sample_rate); //48Hz = 24ppqn @ 120bpm

  touch.Init(hw);
  //term.SetOnNoteOn(OnTerminalNoteOn);
	//term.SetOnNoteOff(OnTerminalNoteOff);
	//term.SetOnScaleSelect(OnScaleSelect);

	arp.SetOnNoteOn(OnArpNoteOn);
	arp.SetOnNoteOff(OnArpNoteOff);

  //Configure input
  mode_switch.Init(Digital::S30, GPIO::Mode::INPUT);

  //Create an ADC configuration
	AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
	adcConfig[speed_knob].InitSingle(Digital::S31);
	adcConfig[length_knob].InitSingle(Digital::S32);
	adcConfig[direction_random_knob].InitSingle(Digital::S33);
  
  //Initialize the adc with the config we just made
  hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);
  hw.adc.Start();

  hw.SetLed(false);

	hw.StartAudio(AudioCallback);

  // Enable Logging, and set up the USB connection.
  hw.StartLog(true);
  hw.PrintLine("Config complete !!!");

  ///////////////////////////////////////////////////////////////
	////////////////////////// LOOP ///////////////////////////////

  while (1) {
    float speed = hw.adc.GetFloat(speed_knob);
		float freq = kMinFreq + kFreqRange * speed;

		metro.SetFreq(freq); 
    
		//hw.SetLed(term.IsLatched());

		//term.Process();
      		
		float arp_lgt = hw.adc.GetFloat(length_knob); //duino analogRead
		float arp_ctr = hw.adc.GetFloat(direction_random_knob); //duino analogRead
		ArpDirection arp_dir = arp_ctr < .5f ? ArpDirection::fwd : ArpDirection::rev;
		float arp_rnd = arp_ctr < .5f ? 2.f * arp_ctr : 2.f * (1.f - arp_ctr);
		arp.SetDirection(arp_dir);
		arp.SetRandChance(arp_rnd);
		//arp.SetAsPlayed(asPlayedSwitch.Read()); //duino digitalRead
		arp.SetNoteLength(arp_lgt);

    System::Delay(4);
  }
}
