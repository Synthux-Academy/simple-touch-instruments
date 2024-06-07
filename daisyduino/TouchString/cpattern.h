////////////////////////////////////////////////////////////
// PATTERN GENERATOR ///////////////////////////////////////
#pragma once
#include <array>

namespace synthux {

class CPattern {
public:
    CPattern():
    _onsets     { 0 },
    _max_onsets { kSize },
    _next_point { 0 },
    _shift      { 0 }
    { }

    void SetShift(const float frac_offset) {
        // 0...half of the pattern
        _shift = static_cast<uint32_t>(round(frac_offset * kSize));
    }

    void SetMaxOnsets(uint32_t max_onsets) {
        //0...16
        _max_onsets = std::min(max_onsets, kSize);
    }

    void SetOnsets(const float frac_onsets) {
        auto onsets { static_cast<uint32_t>(round(frac_onsets * (_max_onsets - 1)) + 1) };
        if (onsets == _onsets) return;
        _onsets = onsets;

        if (onsets == 0) { _pattern.fill(0); return; }
        else if (onsets == kSize) { _pattern.fill(1); return; }

        // Christoffel word algorythm ///////////////////////////////////
        // Derrived from "Creating Rhythms" (ISBN: 9781887187220)
        // by Stefan Hollos and J. Richard Hollos
        auto y = onsets, a = y;
        auto x = kSize - onsets, b = x;
        auto i = 0;
        _pattern[i++] = 1;
        while (a != b) {
            if (a > b) { _pattern[i] = 1; b += x; }
            else { _pattern[i] = 0; a += y; }
            i++;
        }
        _pattern[i++] = 0;

        if (i == kSize) return;
        // In case of relatively non-prime numbers (e.g. 4/16, 8/16 etc.)
        // only part of the slots will be filled, so we need
        // to copy them to fill the entire patterm
        auto offset = i;
        i = 0;
        while (i + offset < kSize) {
            _pattern[i + offset] = _pattern[i];
            i++;
        }
        ///////////////////////////////////////////////////////////////////
    }

    bool Tick() {
        // Apply shift        
        auto point = static_cast<int32_t>(_next_point) - static_cast<int32_t>(_shift);
        if (point < 0) point += kSize;

        // Advance to the next pattern point
        if (++_next_point == kSize) _next_point = 0;

        // Send trigger if pattern point is 1
        return _pattern[point];
    }

    void Reset() {
      _next_point = 0;
    }

private:
    static constexpr uint32_t kSize = 16; // pattern size

    std::array<uint32_t, kSize> _pattern;
    uint32_t _onsets;
    uint32_t _max_onsets;
    uint32_t _next_point;
    uint32_t _shift;
};

};
