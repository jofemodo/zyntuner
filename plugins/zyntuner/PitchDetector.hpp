/*
 * Zyntuner - low CPU monophonic pitch detector (decimated YIN, spread over the audio blocks)
 *
 * Copyright (C) 2026 riban
 * License: GPL-3.0-or-later
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

// RT-safe: fixed buffers, no allocation. Only the anti-alias filter runs per sample.
class PitchDetector
{
public:
    static constexpr float kMinFreq = 28.0f;    // below a 5-string bass B0 (30.9 Hz)
    static constexpr float kMaxFreq = 1350.0f;  // above guitar E6 (1318.5 Hz)

    void setSampleRate(const double sampleRate)
    {
        fDecim = std::max(1, static_cast<int>(sampleRate / 6000.0 + 0.5));
        fRate = static_cast<float>(sampleRate / fDecim);
        fWindow = std::min(kMaxWindow, static_cast<int>(fRate * 0.065f));
        fMaxLag = std::min(kMaxLag - 2, static_cast<int>(fRate / kMinFreq) + 2);
        fHop = std::max(1, static_cast<int>(fRate * 0.06f));
        // 4th order Butterworth low-pass: two biquads
        const double w = 2.0 * M_PI * 1500.0 / sampleRate, c = std::cos(w);
        const double q[2] = { 0.54119610, 1.30656296 };
        for (int s = 0; s < 2; ++s)
        {
            const double alpha = std::sin(w) / (2.0 * q[s]), a0 = 1.0 + alpha;
            fB[s] = static_cast<float>((1.0 - c) * 0.5 / a0);
            fA1[s] = static_cast<float>(-2.0 * c / a0);
            fA2[s] = static_cast<float>((1.0 - alpha) / a0);
        }
        fHpCoef = std::exp(-2.0f * static_cast<float>(M_PI) * 15.0f / fRate);
        reset();
    }

    void reset()
    {
        std::memset(fRing, 0, sizeof(fRing));
        std::memset(fZ, 0, sizeof(fZ));
        fWrite = fPhase = fSinceHop = fFill = 0;
        fHpX = fHpY = 0.0f;
        fBusy = false;
        fMisses = 0;
        fFreq = 0.0f;
    }

    // Detected frequency in Hz, 0 = no pitch
    float frequency() const { return fFreq; }

    void setThreshold(const float db) { fThreshold = std::pow(10.0f, db * 0.1f); }  // mean square

    inline void push(const float x)
    {
        float v = x + 1e-18f;  // keeps the filters out of denormals
        for (int s = 0; s < 2; ++s)
        {
            const float y = fB[s] * v + fZ[s][0];
            fZ[s][0] = 2.0f * fB[s] * v - fA1[s] * y + fZ[s][1];
            fZ[s][1] = fB[s] * v - fA2[s] * y;
            v = y;
        }
        if (++fPhase < fDecim)
            return;
        fPhase = 0;
        fHpY = v - fHpX + fHpCoef * fHpY;  // DC blocker
        fHpX = v;
        fRing[fWrite] = fHpY;
        fWrite = (fWrite + 1) & (kRing - 1);
        fFill = std::min(fFill + 1, kRing);
        ++fSinceHop;
    }

    // Once per block, after pushing its samples: does only this block's share of the analysis
    void process(const uint32_t frames)
    {
        if (!fBusy && fSinceHop >= fHop)
        {
            fSinceHop = 0;
            startFrame();
        }
        if (!fBusy)
            return;
        // Finish within half a hop
        const float share = 2.0f * frames / (static_cast<float>(fHop) * fDecim);
        const int lags = std::max(1, static_cast<int>(std::ceil(share * fMaxLag)));
        const int end = std::min(fMaxLag, fLag + lags);
        for (; fLag <= end; ++fLag)
        {
            const float* const a = fFrame;
            const float* const b = fFrame + fLag;
            float sum = 0.0f;
            for (int i = 0; i < fWindow; ++i)
            {
                const float d = a[i] - b[i];
                sum += d * d;
            }
            fDiff[fLag] = sum;
        }
        if (fLag > fMaxLag)
        {
            fBusy = false;
            finishFrame();
        }
    }

private:
    static constexpr int kRing = 1024;
    static constexpr int kMaxWindow = 520;
    static constexpr int kMaxLag = 300;

    void startFrame()
    {
        const int n = fWindow + fMaxLag;
        if (fFill < n)
            return;
        int r = (fWrite - n) & (kRing - 1);
        float energy = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            fFrame[i] = fRing[r];
            energy += fFrame[i] * fFrame[i];
            r = (r + 1) & (kRing - 1);
        }
        if (energy / n < fThreshold)  // silence: skip the expensive part
        {
            fFreq = 0.0f;
            fMisses = 0;
            return;
        }
        fDiff[0] = 0.0f;
        fLag = 1;
        fBusy = true;
    }

    void finishFrame()
    {
        const float f = analyse();
        if (f <= 0.0f)
        {
            if (++fMisses > 2)  // ignore short drop-outs (pick noise)
                fFreq = 0.0f;
            return;
        }
        fMisses = 0;
        // Smooth small movements, follow note changes at once
        if (fFreq > 0.0f && std::fabs(f / fFreq - 1.0f) < 0.02f)
            fFreq += 0.4f * (f - fFreq);
        else
            fFreq = f;
    }

    // Vertex of the parabola through fDiff[t-1..t+1]
    float vertex(const int t) const
    {
        const float a = fDiff[t - 1], b = fDiff[t], c = fDiff[t + 1];
        const float den = a - 2.0f * b + c;
        if (den <= 0.0f)
            return static_cast<float>(t);
        return t + std::max(-1.0f, std::min(1.0f, 0.5f * (a - c) / den));
    }

    int localMin(const int centre, const int radius) const
    {
        int best = std::max(1, centre - radius);
        const int hi = std::min(fMaxLag - 1, centre + radius);
        for (int t = best + 1; t <= hi; ++t)
            if (fDiff[t] < fDiff[best])
                best = t;
        return best;
    }

    float analyse() const
    {
        // YIN: first dip of the cumulative mean normalised difference below a threshold. The depth is
        // interpolated: for high notes (period of few samples) the dip falls between two lags.
        const int minLag = std::max(2, static_cast<int>(fRate / kMaxFreq));
        float cum = fDiff[1];
        int tau = 0;
        for (int t = 2; t < fMaxLag; ++t)
        {
            const float l = fDiff[t - 1], m = fDiff[t], r = fDiff[t + 1];
            cum += m;
            if (t < minLag || m > l || m >= r || cum <= 0.0f)
                continue;
            const float depth = m - (l - r) * (l - r) / (8.0f * (l - 2.0f * m + r));
            if (depth * t < 0.2f * cum)
            {
                tau = t;
                break;
            }
        }
        if (tau <= 0 || tau >= fMaxLag - 1)
            return 0.0f;

        // Refine on multiples of the period: the interpolation error is divided by the multiple
        float period = vertex(tau);
        int mult = 1;
        while (period * (mult * 2) < fMaxLag - 2)
        {
            mult *= 2;
            const int t = localMin(static_cast<int>(period * mult + 0.5f), 2);
            if (t < 2 || t >= fMaxLag - 1)
                break;
            period = vertex(t) / mult;
        }
        const float freq = fRate / period;
        return freq >= kMinFreq && freq <= kMaxFreq ? freq : 0.0f;
    }

    int fDecim = 8, fWindow = 390, fMaxLag = 216, fHop = 360;
    float fRate = 6000.0f, fThreshold = 1e-6f;
    float fB[2] = {}, fA1[2] = {}, fA2[2] = {}, fZ[2][2] = {};
    float fHpCoef = 0.99f, fHpX = 0.0f, fHpY = 0.0f;
    int fWrite = 0, fPhase = 0, fSinceHop = 0, fFill = 0, fLag = 1, fMisses = 0;
    bool fBusy = false;
    float fFreq = 0.0f;
    float fRing[kRing] = {}, fFrame[kMaxWindow + kMaxLag] = {}, fDiff[kMaxLag] = {};
};
