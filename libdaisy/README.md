# Implementation of the Simple Touch examples in C++

## Purpose

The purpose of this effort is to reimplement the
`daisyduino/simple-*-touch` examples using libDaisy and DaisySP as an
alternative to Daisyduino.

The most intresting changes can be seen in the use of the libDaisy Mpr121I2C for
`simple-daisy-touch.h` and using 
[GPIO](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a1___getting-_started-_g_p_i_o.html)
and [ADC](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a4___getting-_started-_a_d_cs.html) for the digital and analog arduino reads respectively.

* This is an excellent intro to the platform: GPIO, serial printing, ADC
[libDaisy ref docs](https://electro-smith.github.io/libDaisy/index.html)

## Setup

See [DaisyWiki](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) for setting up your development
enviornment.

Install the DaisySP submodules

```bash
git submodule update --init --recursive
cd libdaisy/lib/DaisySP/
make
cd ../libDaisy/
make
```

## Build

For each example directory do the following after 
[Flashing](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4-Run-the-Blink-Example)
the Daisy via USB.

```bash
make clean ; make ; make program-dfu
```