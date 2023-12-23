#include "daisy_seed.h"
#include "daisysp.h"

#include "vox.h"
#include "arp.h"
#include "term.h"
#include "scale.h"

using namespace daisy;
using namespace daisysp;


using namespace synthux;

////////////////////////////////////////////////////////////
////////////////////////// CONTROLS ////////////////////////

#define S30 daisy::seed::D15.pin // SWITCH : ORDERED / AS PLAYED
#define S31 daisy::seed::A1.pin  // KNOB : SPEED
#define S32 daisy::seed::A2.pin  // KNOB : LENGTH
#define S33 daisy::seed::A3.pin  // KNOB : DIRECTION / RANDOM

static const int kAnalogResolution  = 7; //7bits => 0..127
static const float kKnobMax = powf(2.f, kAnalogResolution) - 1.f;

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////

static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;

static Scale scale;
static Terminal term;
static Arp<kNotesCount, kPPQN> arp;
static Metro metro;
static Vox vox;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = kPPQN * kBPMRange / kSecPerMin;


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
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	float sample_rate = hw.AudioSampleRate();
	vox.Init(sample_rate);

	metro.Init(48, sample_rate); //48Hz = 24ppqn @ 120bpm 

	term.Init();
	term.SetOnNoteOn(OnTerminalNoteOn);
	term.SetOnNoteOff(OnTerminalNoteOff);
	term.SetOnScaleSelect(OnScaleSelect);

	arp.SetOnNoteOn(OnArpNoteOn);
	arp.SetOnNoteOff(OnArpNoteOff);

	//Create an ADC configuration
    AdcChannelConfig adcConfig;
    //I think this set these Knobs up for ADC
	adcConfig.InitSingle(hw.GetPin(S30));
    adcConfig.InitSingle(hw.GetPin(S31));
	adcConfig.InitSingle(hw.GetPin(S32));
	adcConfig.InitSingle(hw.GetPin(S33));


	//Led led1;
	//Don't think I need this; led1 = Led(hw.GetPin)
	//Don't think I need this; analogReadResolution(kAnalogResolution);
	//https://electro-smith.github.io/libDaisy/md_doc_2md_2__a4___getting-_started-_a_d_cs.html

	hw.StartAudio(AudioCallback);

	///////////////////////////////////////////////////////////////
	////////////////////////// LOOP ///////////////////////////////

	while(1) {
		float speed = hw.adc.GetFloat(0)  / kKnobMax;
		float freq = kMinFreq + kFreqRange * speed;
		
		metro.SetFreq(freq); 

		hw.SetLed(term.IsLatched());
		
		term.Process();

		float arp_lgt = hw.adc.GetFloat(2) / kKnobMax; //duino analogRead
		float arp_ctr = hw.adc.GetFloat(3) / kKnobMax; //duino analogRead
		ArpDirection arp_dir = arp_ctr < .5f ? ArpDirection::fwd : ArpDirection::rev;
		float arp_rnd = arp_ctr < .5f ? 2.f * arp_ctr : 2.f * (1.f - arp_ctr);
		arp.SetDirection(arp_dir);
		arp.SetRandChance(arp_rnd);
		arp.SetAsPlayed(hw.adc.GetFloat(0)); //duino digitalRead
		arp.SetNoteLength(arp_lgt);

		System::Delay(4);
	}
}
