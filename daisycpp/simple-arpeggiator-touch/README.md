# simple-arpeggiator-touch

## Author

Michael D. Jones

Derived from bleep tools work <https://github.com/Synthux-Academy/simple-examples-touch/tree/main/daisyduino/simple-arpeggiator-touch>

## Description

The purpose of this effort was to reimplement the `simple-arpeggiator-touch`
with C++ and with no use of the arduino libraries. The C++ compile times are 
faster, in this case, and also promote command line build cycles (ex: `make`)
without the need for an IDE.

Initially I had attempted to capture all of my changes by coping unmodified
files from the daisyduino to the daisycpp. And them track each atomic change
with git.

```bash
cp ./daisyduino/simple-arpeggiator-touch/* ./daisycpp/simple-arpeggiator-touch/
```

However, there was a significant update to the upstream repo on Dec. 26, 2023
and I rather frantically re-implemented most things non-atomically. The
most intresting changes can be seen in the use of the libDaisy Mpr121I2C for
`simple-daisy-touch.h` and using 
[GPIO](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a1___getting-_started-_g_p_i_o.html)
and [ADC](https://electro-smith.github.io/libDaisy/md_doc_2md_2__a4___getting-_started-_a_d_cs.html) for the digital and analog arduino reads respectively in `simple-arpeggiator-touch.cpp`.

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

[Flash](https://github.com/electro-smith/DaisyWiki/wiki/1.-Setting-Up-Your-Development-Environment#4-Run-the-Blink-Example)
the Daisy via USB by

```bash
make clean ; make ; make program-dfu
```
