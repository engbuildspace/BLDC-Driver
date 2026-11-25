#pragma once

#include <stdint.h>

// Small replacement for strtof() for the serial console: plain decimal
// numbers only ("12", "-0.5", ".25", "3e-3", "1.5E+2"), no hex floats, inf
// or nan. newlib's strtof() pulls in its full strtod (hex floats, NaN
// payloads, arbitrary-precision maths), about 5-8 KB of flash.
//
// Like strtof(): leading spaces/tabs are skipped, *end is set to the first
// character not used (or to s if there was no number). Up to 9 significant
// digits are kept (more than a float holds); the result is within a few
// units in the last place of the exact value, plenty for settings.
inline float parseFloat(const char *s, char **end) {
    const char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    bool neg = false;
    if (*p == '+' || *p == '-') { neg = (*p == '-'); p++; }

    uint32_t mant = 0;
    int exp10 = 0;
    bool any = false;
    for (; *p >= '0' && *p <= '9'; p++) {
        any = true;
        if (mant < 100000000u) mant = mant * 10 + (uint32_t)(*p - '0');
        else exp10++; // digits past the 9th only scale
    }
    if (*p == '.') {
        p++;
        for (; *p >= '0' && *p <= '9'; p++) {
            any = true;
            if (mant < 100000000u) { mant = mant * 10 + (uint32_t)(*p - '0'); exp10--; }
        }
    }
    if (!any) {
        if (end) *end = const_cast<char *>(s);
        return 0.0f;
    }
    // Exponent, only if digits follow (so "2e" parses as 2 with "e" left over).
    if (*p == 'e' || *p == 'E') {
        const char *q = p + 1;
        bool eneg = false;
        if (*q == '+' || *q == '-') { eneg = (*q == '-'); q++; }
        if (*q >= '0' && *q <= '9') {
            int e = 0;
            for (; *q >= '0' && *q <= '9'; q++) if (e < 1000) e = e * 10 + (*q - '0');
            exp10 += eneg ? -e : e;
            p = q;
        }
    }
    if (end) *end = const_cast<char *>(p);

    if (mant == 0) return neg ? -0.0f : 0.0f; // (0 x inf would be NaN)

    // One rounding for the power of ten (exact in float up to 1e10), then one
    // multiply or divide. Out-of-range values become inf or 0, as strtof does.
    float v = (float)mant;
    if (exp10 > 45) exp10 = 45;
    if (exp10 < -60) exp10 = -60;
    float scale = 1.0f;
    for (int i = exp10 < 0 ? -exp10 : exp10; i > 0; i--) {
        scale *= 10.0f;
        if (exp10 < 0 && scale > 1e30f) { v /= scale; scale = 1.0f; } // keep the scale finite for tiny values
    }
    v = exp10 < 0 ? v / scale : v * scale;
    return neg ? -v : v;
}
