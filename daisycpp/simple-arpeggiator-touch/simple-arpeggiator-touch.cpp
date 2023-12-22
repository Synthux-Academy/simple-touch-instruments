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

#define S30 D15 // SWITCH : ORDERED / AS PLAYED
#define S31 A1  // KNOB : SPEED
#define S32 A2  // KNOB : LENGTH
#define S33 A3  // KNOB : DIRECTION / RANDOM

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
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
	}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAudio(AudioCallback);
	while(1) {}
}
