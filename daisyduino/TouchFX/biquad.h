#pragma once
#ifndef INFS_BIQUADFILTERS_H
#define INFS_BIQUADFILTERS_H

#include <cassert>
#include <array>
#include "DaisyDSP.h"

namespace infrasonic {

/**
 *  Single precision 2nd order biquad filter section with support for stereo processing.
 *  NOTE: There are templated filter definitions at the bottom of this file for convenience. 
 */
class BiquadSection {

    public:

        enum class FilterType {
            LowPass,
            HighPass,
            BandPass
        };

        // Assumes normalized coefficients (a0 == 1)
        // Ordering numerator then denominator: {b0, b1, b2, a1, a2}
        using Coefficients = std::array<float, 5>;

        static const Coefficients CalculateCoefficients(const FilterType type, 
                                                        const float sample_rate,
                                                        const float cutoff_hz,
                                                        const float q);

        BiquadSection() {}
        ~BiquadSection() {}

        void SetCoefficients(const Coefficients coefficients) { coefs_ = coefficients; }

        inline float Process(const float in, const int channel)
        {
            assert(channel < 2);

            // TODO: arm accelerated version

            const float &b0 = coefs_[0];
            const float &b1 = coefs_[1];
            const float &b2 = coefs_[2];
            const float &a1 = coefs_[3];
            const float &a2 = coefs_[4];

            // Transposed direct form 2
            float y = b0 * in + s1_[channel];
            s1_[channel] = s2_[channel] + in * b1 - a1 * y;
            s2_[channel] = b2 * in - a2 * y;
            return y;
        }

    private:
        // coef
        Coefficients coefs_{0, 0, 0, 0, 0};

        // state
        float s1_[2] = {0, 0};
        float s2_[2] = {0, 0};

};

/// Templated cascaded biquad filter. Filter Order = 2 * NumSections.
/// NOTE: Only supports even-ordered filters.
template<size_t NumSections, BiquadSection::FilterType FilterType>
class BiquadCascade {

    static_assert(NumSections > 0, "Must have at least one section");

    public:
        BiquadCascade() {}
        ~BiquadCascade() {}

        // Individual param update methods recalculate coefficients
        // every time one of them is called. To update everything at once,
        // use SetParams()

        void Init(const float sample_rate) {
            sample_rate_ = sample_rate;
            cutoff_hz_ = sample_rate * 0.25f;
            SetFlatResponse();
        }

        inline void SetCutoff(const float cutoff_hz)
        {
            cutoff_hz_ = daisysp::fclamp(cutoff_hz, 1.f, sample_rate_ * 0.5f);
            updateCoefficients();
        }

        inline void SetQ(const float q)
        {
            for (size_t i=0; i<NumSections; i++) {
                q_[i] = daisysp::fmax(q, 0.1f);
            }
            updateCoefficients();
        }

        /// Update params simultaneously, recalculating coefficients only once 
        inline void SetParams(const float cutoff_hz, const float q)
        {
            cutoff_hz_ = daisysp::fclamp(cutoff_hz, 1.f, sample_rate_ * 0.5f);
            for (size_t i=0; i<NumSections; i++) {
                q_[i] = daisysp::fmax(q, 0.1f);
            }
            updateCoefficients();
        }

        /// Sets Q values in each section for a truly "flat" (-3dB cutoff point) response
        /// NOTE: per limitations of this class this only works for even-order filters
        inline void SetFlatResponse()
        {
            const float angleIncrement = 1.f / (4.f * static_cast<float>(NumSections));
            for (size_t i=0; i<NumSections; i++) {
                q_[i] = 1.f / (2.f * cosf(PI_F * (angleIncrement * (i * 2 + 1))));
            }
            updateCoefficients();
        }

        inline float Process(const float in)
        {
            float out = in;
            for (auto &biquad : biquads_) {
                out = biquad.Process(out, 0);
            }
            return out;
        }

        /// In-place stereo processing
        inline void ProcessStereo(float &sampL, float &sampR)
        {
            for (auto &biquad : biquads_) {
                sampL = biquad.Process(sampL, 0);
                sampR = biquad.Process(sampR, 1);
            }
        }

    private:

        float sample_rate_;
        float cutoff_hz_, q_[NumSections];

        std::array<BiquadSection, NumSections> biquads_;

        inline void updateCoefficients() {
            for (size_t i=0; i<NumSections; i++) {
                biquads_[i].SetCoefficients(BiquadSection::CalculateCoefficients(FilterType, sample_rate_, cutoff_hz_, q_[i]));
            }
        }
};

/// 12dB/Oct resonant lowpass filter
using LPF12 = BiquadCascade<1, BiquadSection::FilterType::LowPass>;

/// 24dB/Oct resonant lowpass filter
using LPF24 = BiquadCascade<2, BiquadSection::FilterType::LowPass>;

/// 12dB/Oct resonant highpass filter
using HPF12 = BiquadCascade<1, BiquadSection::FilterType::HighPass>;

/// 24dB/Oct resonant highpass filter
using HPF24 = BiquadCascade<2, BiquadSection::FilterType::HighPass>;

/// 12dB/Oct resonant bandpass filter
using BPF12 = BiquadCascade<1, BiquadSection::FilterType::BandPass>;

/// 24dB/Oct resonant bandpass filter
using BPF24 = BiquadCascade<2, BiquadSection::FilterType::BandPass>;

}

#endif