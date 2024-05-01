#include "biquad.h"
#include <cmath>
#include "dsputils.h"

using namespace infrasonic;

const BiquadSection::Coefficients BiquadSection::CalculateCoefficients(const FilterType type, 
                                                                       const float sample_rate,
                                                                       const float cutoff_hz,
                                                                       const float q)
{
   Coefficients coefs; 
    float norm;
    const float K = infrasonic::tanf(PI_F * cutoff_hz * (1.0f / sample_rate));
    const float Ksq = K * K;

    float &b0 = coefs[0];
    float &b1 = coefs[1];
    float &b2 = coefs[2];
    float &a1 = coefs[3];
    float &a2 = coefs[4];

    switch (type) {
        case FilterType::HighPass:
            norm = 1.0f / (1.0f + (K / q) + Ksq);
            b0 = norm;
            b1 = -2.0f * b0;
            b2 = b0;
            a1 = 2.0f * (Ksq - 1.0f) * norm;
            a2 = (1.0f - (K / q) + Ksq) * norm; 
            break;

        case FilterType::BandPass:
            norm = 1.0f / (1.0f + (K / q) + Ksq);
            b0 = (K / q) * norm;
            b1 = 0.0f;
            b2 = -b0;
            a1 = 2.0f * (Ksq - 1) * norm;
            a2 = (1.0f - (K / q) + Ksq) * norm;
            break;

        default: // lowpass
            norm = 1.0f / (1.0f + (K / q) + Ksq);
            b0 = Ksq * norm;
            b1 = 2.0f * b0;
            b2 = b0;
            a1 = 2.0f * (Ksq - 1.0f) * norm;
            a2 = (1.0f - (K / q) + Ksq) * norm;
            break;
    }

   return coefs;
}
