# Implementation of the Simple Touch examples in C++

## Purpose

The purpose of this effort is to reimplement the
`daisyduino/simple-*-touch` examples using C++ and no use of the arduino
libraries. The C++ compile times are faster, in this case, and also promote
command line build cycles (ex: `make`) without the need for an IDE.

The most intresting changes can be seen in the use of the libDaisy Mpr121I2C for
`simple-daisy-touch.h` and using 
[GPIO](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a1___getting-_started-_g_p_i_o.html)
and [ADC](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a4___getting-_started-_a_d_cs.html) for the digital and analog arduino reads respectively.

## Build

See [DaisyWiki](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment) for setting up your development
enviornment. 

Make sure the tool chain is in your path and the LIBDAISY_DIR/DAISYSP_DIR
variables are set.

```bash
export LIBDAISY_DIR=${HOME}/Builds/libDaisy/
export DAISYSP_DIR=${HOME}/Builds/DaisySP/

PATH=$PATH:/Library/DaisyToolchain/0.2.0/arm/bin/
PATH=$PATH:/Library/DaisyToolchain/0.2.0/bin/
```

For each example directory do the following after 
[Flashing](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4-Run-the-Blink-Example)
the Daisy via USB.

```bash
make clean ; make ; make program-dfu
```