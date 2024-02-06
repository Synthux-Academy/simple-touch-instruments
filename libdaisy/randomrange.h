#pragma once
#include <random>

class Randomrange {
public:
    uint32_t generate(uint32_t start, uint32_t stop) {
        std::uniform_int_distribution<uint32_t> distribution(start, stop);
        return distribution(generator);
    }   

private:
    std::random_device randomDevice;
    unsigned seed = randomDevice();
    std::default_random_engine generator{seed};
};