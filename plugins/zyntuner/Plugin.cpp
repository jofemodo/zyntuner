/*
 * zyntuner - DSP
 * Low CPU guitar tuner with stereo pass-through
 *
 * Copyright (C) 2026 riban
 * License: GPL-3.0-or-later
 */

#include "DistrhoPlugin.hpp"
#include "ParamDefs.hpp"
#include "PitchDetector.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

START_NAMESPACE_DISTRHO

class ZyntunerPlugin : public Plugin
{
public:
    ZyntunerPlugin()
        : Plugin(kParamCount, 0, 0)  // parameters, programs, states
    {
        for (uint32_t i = 0; i < kParamCount; ++i)
            fValues[i] = kParams[i].def;
        sampleRateChanged(getSampleRate());
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // Information

    const char* getLabel() const override { return "zyntuner"; }
    const char* getDescription() const override { return "Low CPU guitar tuner with stereo pass-through"; }
    const char* getMaker() const override { return "riban"; }
    const char* getHomePage() const override { return "https://zynthian.org"; }
    const char* getLicense() const override { return "GPL-3.0-or-later"; }
    uint32_t getVersion() const override { return d_version(0, 1, 0); }

    // ----------------------------------------------------------------------------------------------------------------
    // Ports, parameters and groups (from ParamDefs.hpp)

    void initPortGroup(const uint32_t groupId, PortGroup& portGroup) override
    {
        if (groupId < kGroupCount)
        {
            portGroup.name = kGroups[groupId].name;
            portGroup.symbol = kGroups[groupId].symbol;
        }
    }

    void initParameter(const uint32_t index, Parameter& parameter) override
    {
        if (index >= kParamCount)
            return;
        const ParamDef& p(kParams[index]);
        if (p.designation != kParameterDesignationNull)
            parameter.initDesignation(p.designation);  // bypass: LV2 port "lv2_enabled", inverted by DPF
        parameter.hints = p.hints;
        parameter.name = p.name;
        parameter.symbol = p.symbol;
        parameter.unit = p.unit;
        parameter.ranges.min = p.min;
        parameter.ranges.max = p.max;
        parameter.ranges.def = p.def;
        parameter.groupId = p.group >= 0 ? static_cast<uint32_t>(p.group) : kPortGroupNone;
        if (p.enumCount > 0)
        {
            ParameterEnumerationValue* const values = new ParameterEnumerationValue[p.enumCount];
            for (uint32_t i = 0; i < p.enumCount; ++i)
            {
                values[i].label = p.enumLabels[i];
                values[i].value = p.min + i;
            }
            parameter.enumValues.count = p.enumCount;
            parameter.enumValues.restrictedMode = true;
            parameter.enumValues.values = values;
        }
    }

    float getParameterValue(const uint32_t index) const override
    {
        return index < kParamCount ? fValues[index] : 0.0f;
    }

    void setParameterValue(const uint32_t index, const float value) override
    {
        if (index < kParamCount && (kParams[index].hints & kParameterIsOutput) == 0)
            fValues[index] = value;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Processing

    void sampleRateChanged(const double sampleRate) override
    {
        fDetector.setSampleRate(sampleRate);
        fDetector.setThreshold(fValues[kParamThreshold]);
    }

    void activate() override
    {
        fDetector.reset();
        fLastFreq = -1.0f;
        clearOutputs();
    }

    void run(const float** const inputs, float** const outputs, const uint32_t frames) override
    {
        const bool bypassed = fValues[kParamBypass] > 0.5f;
        const float* const inL = inputs[0];
        const float* const inR = inputs[1];
        float* const outL = outputs[0];
        float* const outR = outputs[1];

        // The audio is never modified, so bypass needs no crossfade: it only stops the detection
        for (uint32_t i = 0; i < frames; ++i)
        {
            const float l = inL[i], r = inR[i];
            outL[i] = l;
            outR[i] = r;
            if (!bypassed)
                fDetector.push(0.5f * (l + r));
        }

        if (bypassed)
        {
            if (!fWasBypassed)
            {
                fDetector.reset();
                fLastFreq = -1.0f;
                clearOutputs();
            }
            fWasBypassed = true;
            return;
        }
        fWasBypassed = false;

        if (fValues[kParamThreshold] != fLastThreshold)
        {
            fLastThreshold = fValues[kParamThreshold];
            fDetector.setThreshold(fLastThreshold);
        }
        fDetector.process(frames);
        updateOutputs();
    }

private:
    void clearOutputs()
    {
        fValues[kParamNote] = fValues[kParamCents] = fValues[kParamValid] = 0.0f;
    }

    void updateOutputs()
    {
        const float freq = fDetector.frequency(), ref = fValues[kParamReference];
        if (freq == fLastFreq && ref == fLastRef)
            return;
        fLastFreq = freq;
        fLastRef = ref;
        if (freq <= 0.0f)
        {
            fValues[kParamValid] = 0.0f;  // note and cents keep their last values
            return;
        }
        const float pitch = 69.0f + 12.0f * std::log2(freq / ref);
        const float note = std::floor(pitch + 0.5f);
        fValues[kParamNote] = std::max(0.0f, std::min(127.0f, note));
        fValues[kParamCents] = (pitch - note) * 100.0f;
        fValues[kParamValid] = 1.0f;
    }

    float fValues[kParamCount];
    PitchDetector fDetector;
    float fLastFreq = -1.0f, fLastRef = 0.0f, fLastThreshold = 1.0f;
    bool fWasBypassed = false;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZyntunerPlugin)
};

Plugin* createPlugin()
{
    return new ZyntunerPlugin();
}

END_NAMESPACE_DISTRHO
