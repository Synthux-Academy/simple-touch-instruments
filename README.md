# Simple Touch Synth

Explore various touch-controlled instruments and examples, designed for the Simple Touch Synth board and kit. Grab a kit in our shop to get started quickly and support the project! https://www.synthux.academy/shop

![Simple Touch Synth](https://github.com/Synthux-Academy/simple-touch-instruments/assets/91409567/21892c99-8bf1-432e-a098-692aca500b3c)



* This is an excellent intro to the platform! GPIO, serial printing, ADC.
[libDaisy ref docs](https://electro-smith.github.io/libDaisy/index.html)

## Setup

If you plan on using the libdaisy implementations there are few extra steps

```bash
git submodule update --init --recursive
cd libdaisy/lib/DaisySP/
make
cd ../libDaisy/
make
```
