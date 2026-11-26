/*
 * zyntuner - parameters and port groups, shared by DSP (Plugin.cpp) and GUI (UI.cpp).
 * Symbols are stable API (presets, snapshots, MIDI learn, zynthian widget): don't rename them lightly.
 * Control pages in zynthian = port groups (4 controls per page).
 */

#pragma once

#include "DistrhoDetails.hpp"
#include "Scales.hpp"

START_NAMESPACE_DISTRHO

enum ParamId : uint32_t {
    kParamReference = 0,
    kParamThreshold = 1,
    kParamBypass = 2,
    kParamScale = 3,
    kParamNote = 4,  // output
    kParamCents = 5,  // output
    kParamValid = 6,  // output
    kParamDegree = 7,  // output
    kParamOctave = 8,  // output
    kParamCount
};

enum GroupId : uint32_t {
    kGroupTuner = 0,
    kGroupCount
};

struct GroupDef {
    const char* symbol;
    const char* name;
};

struct ParamDef {
    const char* symbol;
    const char* name;
    const char* unit;
    float min, max, def;
    uint32_t hints;
    int32_t group;  // GroupId or -1
    const char* const* enumLabels;
    uint32_t enumCount;
    ParameterDesignation designation;  // kParameterDesignationBypass: 1 = bypassed (LV2 port lv2_enabled)
};


static const GroupDef kGroups[kGroupCount > 0 ? static_cast<uint32_t>(kGroupCount) : 1u] = {
    { "tuner", "Tuner" },
};

static const ParamDef kParams[kParamCount] = {
    { "reference", "A4 reference", "Hz", 415.0f, 466.0f, 440.0f, kParameterIsAutomatable, 0, nullptr, 0, kParameterDesignationNull },
    { "threshold", "Threshold", "dB", -80.0f, -20.0f, -60.0f, kParameterIsAutomatable, 0, nullptr, 0, kParameterDesignationNull },
    { "bypass", "Bypass", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsBoolean, 0, nullptr, 0, kParameterDesignationBypass },
    { "scale", "Scale", "", 0.0f, kScaleCount - 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsInteger, 0, kScaleLabels, kScaleCount, kParameterDesignationNull },
    { "note", "MIDI note", "", 0.0f, 127.0f, 0.0f, kParameterIsAutomatable | kParameterIsInteger | kParameterIsOutput, -1, nullptr, 0, kParameterDesignationNull },
    { "cents", "Cents", "", -150.0f, 150.0f, 0.0f, kParameterIsAutomatable | kParameterIsOutput, -1, nullptr, 0, kParameterDesignationNull },
    { "valid", "Pitch valid", "", 0.0f, 1.0f, 0.0f, kParameterIsAutomatable | kParameterIsBoolean | kParameterIsOutput, -1, nullptr, 0, kParameterDesignationNull },
    { "degree", "Scale degree", "", 0.0f, 63.0f, 0.0f, kParameterIsAutomatable | kParameterIsInteger | kParameterIsOutput, -1, nullptr, 0, kParameterDesignationNull },
    { "octave", "Octave", "", -1.0f, 10.0f, 4.0f, kParameterIsAutomatable | kParameterIsInteger | kParameterIsOutput, -1, nullptr, 0, kParameterDesignationNull },
};

// Bypass parameter: the audio is muted (soft) while the tuner is enabled and passes while bypassed
static const uint32_t kBypassParam = kParamBypass;

// Outputs filled by the demo meters in Plugin.cpp (peak dB of the audio outputs)
static const uint32_t kMeterCount = 0;
static const uint32_t kMeterParams[kMeterCount > 0 ? kMeterCount : 1] = { 0 };

END_NAMESPACE_DISTRHO
