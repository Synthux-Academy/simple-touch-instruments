// SYNTHUX ACADEMY /////////////////////////////////////////
// ARPEGGIATED SYNTH ///////////////////////////////////////

#include "simple-daisy-touch.h"
#include "clk.h"
#include "aknob.h"
#include "vox.h"
#include "arp.h"
#include "scale.h"


using namespace synthux;

////////////////////////////////////////////////////////////
<<<<<<< HEAD
/////////////////////// CONSTANTS //////////////////////////
=======
////////////////////////// CONTROLS ////////////////////////

#define S30 D15 // SWITCH : ORDERED / AS PLAYED
#define S31 A1  // KNOB : SPEED
#define S32 A2  // KNOB : LENGTH
#define S33 A3  // KNOB : DIRECTION / RANDOM

static const int kAnalogResolution  = 7; //7bits => 0..127
static const float kKnobMax = powf(2.f, kAnalogResolution) - 1.f;

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////
>>>>>>> 5a0f76a (Change switch back to S30)

static const uint8_t kAnalogResolution = 7; //7bits => 0..127
static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;
static const uint16_t kFirstNotePad = 3;
static const uint16_t kLatchPad = 2;

////////////////////////////////////////////////////////////
//////////////// KNOBS, SWITCHES and JACKS /////////////////

static AKnob<kAnalogResolution> speed_knob(A(S30));
static AKnob<kAnalogResolution> length_knob(A(S32));
static AKnob<kAnalogResolution> direction_random_knob(A(S33));

static const int mode_switch = D(S07);
static const int scale_switch_a = D(S09);
static const int scale_switch_b = D(S10);

// Comment this if you're not
// planning using external sync
#define EXTERNAL_SYNC

#ifdef EXTERNAL_SYNC
static const int clk_pin = D(S31);
#endif

////////////////////////////////////////////////////////////
//////////////////////// MODULES ///////////////////////////

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
      //Drop all latched notes except ones being touched
      for (auto i = 0; i < hold.size(); i++) {
        if (hold[i] && !touch.IsTouched(i + kFirstNotePad))  {
          arp.NoteOff(i);
        }
      }
      //Reset latch memory
      hold.fill(false);
    }
    return;
  }

  //Notes
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
  auto note_num = pad - kFirstNotePad;
  if (latch && hold[note_num]) {
    arp.NoteOff(note_num);
    hold[note_num] = false;
  }
  else {
    arp.NoteOn(note_num, 127);
    hold[note_num] = true;
  }
}
void OnPadRelease(uint16_t pad) {
  if (pad < kFirstNotePad || pad >= kFirstNotePad + kNotesCount) return;
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
void OnClockTick() { 
  arp.Trigger(); 
}

///////////////////////////////////////////////////////////////
///////////////////// AUDIO CALLBACK //////////////////////////
void AudioCallback(float **in, float **out, size_t size) {
  clck.Tick();
  for (size_t i = 0; i < size; i++) {
    out[0][i] = out[1][i] = vox.Process();
  }
}

///////////////////////////////////////////////////////////////
////////////////////////// SETUP //////////////////////////////

void setup() {  
  DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  DAISY.SetAudioBlockSize(4);
  auto sample_rate = DAISY.get_samplerate();
  auto buffer_size = DAISY.AudioBlockSize();

  Serial.begin(9600);

  clck.Init(sample_rate, buffer_size);
  clck.SetOnTick(OnClockTick);
  #ifdef EXTERNAL_SYNC
  pinMode(clk_pin, INPUT);
  #endif
  clck.Run();

  touch.Init();
  touch.SetOnTouch(OnPadTouch);
  touch.SetOnRelease(OnPadRelease);

  arp.SetOnNoteOn(OnArpNoteOn);
  arp.SetOnNoteOff(OnArpNoteOff);
  
  vox.Init(sample_rate);

  pinMode(mode_switch, INPUT_PULLUP);
  pinMode(scale_switch_a, INPUT_PULLUP);
  pinMode(scale_switch_b, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  analogReadResolution(kAnalogResolution);

  DAISY.begin(AudioCallback);
}

///////////////////////////////////////////////////////////////
////////////////////////// LOOP ///////////////////////////////

void loop() {
  auto tempo = speed_knob.Process();
  clck.SetTempo(tempo);
   #ifdef EXTERNAL_SYNC
  clck.Process(!digitalRead(clk_pin));
  #endif

  digitalWrite(LED_BUILTIN, latch);

  touch.Process();

  auto arp_length = length_knob.Process();
  auto arp_rand_dir = direction_random_knob.Process();
  auto arp_direction = arp_rand_dir < .5f ? ArpDirection::fwd : ArpDirection::rev;
  auto arp_random = arp_rand_dir < .5f ? 2.f * arp_rand_dir : 2.f * (1.f - arp_rand_dir);
  arp.SetDirection(arp_direction);
  arp.SetRandChance(arp_random);
  arp.SetAsPlayed(digitalRead(mode_switch));
  arp.SetNoteLength(arp_length);

  auto scale_a_val = static_cast<uint8_t>(!digitalRead(scale_switch_a));
  auto scale_b_val = static_cast<uint8_t>(!digitalRead(scale_switch_b));
  scale.SetScaleIndex(scale_a_val + scale_b_val);

  delay(4);
}