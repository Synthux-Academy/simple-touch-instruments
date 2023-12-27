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

> **_NOTE:_**  I am not a regular C++ programing so please forgive and correct
> deviations from best practices. I hacked my way through much of this.

> **_NOTE:_**  I don't think the TEST_KNOBS or TEST_SWITCHES functions are
> working. I am not sure how to use them to test them out.

## Build

```bash
make clean ; make ; make program-dfu
```
