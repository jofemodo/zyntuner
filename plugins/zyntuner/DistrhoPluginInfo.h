/*
 * zyntuner - plugin description macros (DPF)
 * See the DistrhoPluginInfo.h of the examples in dpf/examples for all the available macros.
 */

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_NAME  "zyntuner"
#define DISTRHO_PLUGIN_URI   "urn:riban:tuner"
#define DISTRHO_PLUGIN_BRAND_ID  Zynt
#define DISTRHO_PLUGIN_UNIQUE_ID zTun

#define DISTRHO_PLUGIN_NUM_INPUTS   2
#define DISTRHO_PLUGIN_NUM_OUTPUTS  2
#define DISTRHO_PLUGIN_IS_SYNTH     0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  0
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0
#define DISTRHO_PLUGIN_IS_RT_SAFE   1

#define DISTRHO_PLUGIN_LV2_CATEGORY "lv2:AnalyserPlugin"

// Native GUI: DGL with the Cairo backend (X11UI on Linux)
#define DISTRHO_PLUGIN_HAS_UI       1
#define DISTRHO_UI_USE_CAIRO        1
#define DISTRHO_UI_DEFAULT_WIDTH    640
#define DISTRHO_UI_DEFAULT_HEIGHT   320
#define DISTRHO_UI_USER_RESIZABLE   1

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
