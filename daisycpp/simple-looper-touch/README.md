# simple-looper-touch

## Author

Michael D. Jones

Derived from bleep tools work <https://github.com/Synthux-Academy/simple-examples-touch/tree/main/daisyduino/simple-looper-touch>

## Description

The purpose of this effort was to reimplement the `simple-looper-touch`
with C++ and with no use of the arduino libraries. The C++ compile times are 
faster, in this case, and also promote command line build cycles (ex: `make`)
without the need for an IDE.

## Build

```bash
make clean ; make ; make program-dfu
```

## TODO

