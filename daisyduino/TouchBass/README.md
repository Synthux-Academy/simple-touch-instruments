# THIS IS SIMPLE BASS

# ATTENTION 👀
# THIS IS NOT THE ORIGINAL REPO! 

**[Click here if you came here looking for the official Synthux Github](https://github.com/Synthux-Academy/simple-touch-instruments)**

I'm tweaking clock and scales for now

- editing scale.h
- adding clock

more info at discord

## SCALES

I added these scales in scale.h.
Changed the number of scales to 6

- scales are only relevant to the arpeggiation
- only 7 pads are used
- if changing amount change all items in the array equally

```arduino
{ 130.81, 146.83, 164.81, 174.61, 196.00, 220.00, 246.94, 261.63 }, // C major C3, D3, E3, F3, G3, A3, B3, C4
{ 130.81, 146.83, 155.56, 174.61, 196.00, 207.65, 233.08, 261.63 }, // C minor C3, D3, D#3, F3, G3, G#3, A#3, C4
{ 174.61, 196.00, 207.65, 233.08, 261.63, 277.18, 311.13, 349.23 } // F minor F3, G3, G#3, A#3, C4, C#4, D#4, F4
```
## CLOCK

# Adding Clock

I was looking into adding clock for my ‘modded’ Simple Touch 2, which has a Denki-oto midi trs breakout board.

## Midi vs. clock

Thus far implementing midi clock did not work with my first trials, however clock sync is already in the code for regular clock syncing via the ‘classic’ way. Many synths have a mono jack out labeled as ‘clock’ (e.g. Volca, Plinky, …).

As I informed about this over at Discord Vlad pointed to the fact that syncing is already in the code, to get it working for midi would require some tweaking. It’s the tweaking I wasn’t successful at. 

Vlad’s answer on Discord

> It can be synced already. If you open the code of the bass, string, drum machine or slicer in the Arduino IDE, you'll find there
`// Uncomment this if you're
// planning using external sync
// #define EXTERNAL_SYNC`
> 

> You'll need to define which pin will be used for clock in. It's currently defined as `static const int clk_pin = D(S31);` but needs to be changed as S31 is occupied with the pot.
Syncing to the clock kicks in when tempo is set to minimum.
Clock signal of course should be in Daisy's tolerance range, i.e. 0..+5V. I'm using Korg Volca Sample and Erica Pico Trigger module as sources. KeyStep also would do.
> 

See this discussion on Discord 

[Discord - Group Chat That’s All Fun & Games](https://discord.com/channels/802197755442626590/963902925724344330/1242440673043677225)

## Clock sync is already in the code!

By uncommenting a line and adding the pin on which the clock is connected clock sync works.

Pin S31 is already used for a pot, so I hooked up one of the other free pins I have at my disposal. (few are used for LED’s)

Adding a mono jack to pin S43 = D28 (connecting it to the pin and to ground).

Just need to choose whether to glue it or add a little mount 😅

![Untitled](https://prod-files-secure.s3.us-west-2.amazonaws.com/183357c7-1e01-4c65-a869-200d788b649c/57c6a11f-1370-4e9e-bb63-005610cb452b/Untitled.png)

### Adding the pin in simple-daisy-touch.h - change line 103:

`S35 = D20` by adding a comma and adding the line with the added pin `S43 = D28`

```arduino
// OLD
enum class Digital {
    S07 = D6,
    S08 = D7,
    S09 = D8,
    S10 = D9,
    S30 = D15,
    S31 = D16,
    S32 = D17,
    S33 = D18,
    S34 = D19,
    S35 = D20
};
```

```arduino
// NEW
enum class Digital {
    S07 = D6,
    S08 = D7,
    S09 = D8,
    S10 = D9,
    S30 = D15,
    S31 = D16,
    S32 = D17,
    S33 = D18,
    S34 = D19,
    S35 = D20,
    S43 = D28

};
```

![Untitled](https://prod-files-secure.s3.us-west-2.amazonaws.com/183357c7-1e01-4c65-a869-200d788b649c/ba4261c7-af82-45a6-ad42-efe106a8fda7/Untitled.png)

### In the TouchBass.ino file change line 62 - 65 :

uncomment line 62

In line 65 I changed pin `S31` to `S43`

![Untitled](https://prod-files-secure.s3.us-west-2.amazonaws.com/183357c7-1e01-4c65-a869-200d788b649c/975a2e14-0a92-4835-9bbb-0664e4634ca5/Untitled.png)

## How to use it?

Turn the tempo all the way down, this will now ‘engage’ external sync!

Use touchpads P10 and use either P0 or P02 to change the tempo. Keep pushing multiple times on P0 and you’ll enter external sync.

- P10 + P0/P02 - arpeggiator speed +/-


## QUICK INSTALL
Download the [Binary file](https://github.com/Synthux-Academy/simple-touch-instruments/raw/main/daisyduino/TouchBass/TouchBass.bin) and flash using the [Daisy Seed web programmer](https://electro-smith.github.io/Programmer/)

## CONTROLS
<img src="../../touch.jpeg" width="300"/>

**Switches**
- S07-S08 - arpeggiator off/on/latch
- S09-S10 - Osc 2 mode: \\\ → sound, || and // → AM.     

**Knobs**
- S30 - Osc 2 amount
- S31 - Osc 1 pitch +/- 1 octave
- S31 + P10 - Osc 1 waveform saw/square
- S32 - Osc 2 pitch +/- 1 octave
- S33 - pattern
- S34 - pitch randomisation
- S35 - envelope randomisation
- S35 + P10 - reverb mix
- S36 - filter cutoff
- S36 + P10 - filter resonance
- S36 + P11 - filter envelope amount
- S37 - envelope

**Pads**
- P10 + P0/P02 - arpeggiator speed +/-
- P11 + P0/P02 - scale (one of the three)
- P03...P09 - notes
- P10 + P11 - monophonic / paraphonic mode
  
