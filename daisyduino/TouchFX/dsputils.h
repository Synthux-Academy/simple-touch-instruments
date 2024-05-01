#pragma once
#ifndef INFS_DSPUTILS_H
#define INFS_DSPUTILS_H

#include <cmath>
#include "DaisyDSP.h"
#ifdef __arm__
#include <arm_math.h>
#endif

namespace infrasonic {

inline float dbfs2lin(float dbfs) {
    return daisysp::pow10f(dbfs * 0.05f);
}

inline float lin2dbfs(float lin) {
    return daisysp::fastlog10f(lin) * 20.0f;
}

// Coefficient for one pole smoothing filter based on Tau time constant for `time_s`
inline float onepole_coef(float time_s, float sample_rate) {
    if (time_s <= 0.0f || sample_rate <= 0.0f) { return 1.0f; }
    return daisysp::fmin(1.0f / (time_s * sample_rate), 1.0f);
}

inline float onepole_coef_t60(float time_s, float sample_rate)
{
	return onepole_coef(time_s * 0.1447597f, sample_rate);
}

inline float ftension(const float in, const float factor)
{
    if (factor == 0.0f) return in;
    const float denom = expm1f(factor);
    return expm1f(in * factor) / denom;
}

inline float tanf(const float x)
{
#ifdef __arm__
    return std::tan(x);
    // float s, c;
    // arm_sin_cos_f32(x, &s, &c);
    // return s / c;
#else
    return std::tanf(x);
#endif
}

}

#endif
