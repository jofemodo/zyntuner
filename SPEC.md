# zyntuner: specification

Living document: each decision made in the chat is written here before it's implemented.
Status of each section: TODO / AGREED / DONE (implemented and tested on a zynthian).

## Identity — AGREED
- Name: zyntuner · symbol: `zyntuner` · URI: `urn:riban.tuner`
- Maker: riban · license: GPL-3.0-or-later · LV2 category: lv2:AnalyserPlugin
- Purpose: Low CPU tuner with soft mute and microtonal scales

## I/O — AGREED
- Audio: 2 in, 2 out
- MIDI: no input, no output

## Controls (LV2 input ports) — implemented, not tested on a zynthian
Pages = port groups, 4 controls per page in zynthian. Symbols are stable API.

| Page | Symbol | Name | Range | Default | Unit | Type | Designation / notes |
|---|---|---|---|---|---|---|---|
| Tuner | `reference` | A4 reference | 415 .. 466 | 440 | Hz | float |  |
| Tuner | `threshold` | Threshold | -80 .. -20 | -60 | dB | float |  |
| Tuner | `bypass` | Bypass | 0 .. 1 | 0 |  | boolean | bypass |
| Tuner | `scale` | Scale | 0 .. 17 | 0 (12-TET) |  | enum | from `zyngui/zyntuner_scales.py`, new scales appended only |

## Outputs (LV2 output ports → widget monitors, ~30 Hz) — implemented, not tested on a zynthian

| Symbol | Name | Range | Unit | Meaning / how it's computed |
|---|---|---|---|---|
| `note` | MIDI note | 0 .. 127 |  | nearest MIDI note; keeps its last value when not valid |
| `note` | (see above) | | | since 0.2: nearest 12-TET MIDI note of the target scale degree |
| `cents` | Cents | -150 .. 150 |  | deviation from the nearest degree of the selected scale |
| `valid` | Pitch valid | 0 .. 1 |  |  1 = pitch detected, 0 = silence / no pitch / bypassed |
| `degree` | Scale degree | 0 .. 63 |  | nearest degree (0 = root); keeps its last value when not valid |
| `octave` | Octave | -1 .. 10 |  | octave (tritave for Bohlen-Pierce) of `degree`, 4 = the root's at C4 |

## DSP — implemented, not tested on a zynthian
- Audio: soft mute. While enabled the output fades out (10 ms linear) and stays silent; while bypassed it fades
  in and then copies the input unchanged. No latency. Bypass also stops the detection.
- Scales (`zyngui/zyntuner_scales.py`, also imported by the widget; `make` generates `plugins/zyntuner/Scales.hpp`):
  each is a list of degrees in cents above the root and a period (1200, or the 3:1 tritave). Western tunings put A4
  at the reference; the others put the root at the 12-TET C4 of the reference. 18 scales: 12-TET, just (C),
  Pythagorean (C), 1/4-comma meantone, Werckmeister III, Kirnberger III, Vallotti, 19/24/31/53-TET, 22 shruti,
  slendro, pelog, Thai 7-TET, harmonics 8-15, Partch 43, Bohlen-Pierce. No key/root control (would need a 2nd page).
- Pitch → nearest degree: cents above the root, split into period + remainder, nearest degree (root of the next
  period included).
- Detection on 0.5 * (L + R): 4th order 1.5 kHz low-pass, decimation to ~6 kHz, DC blocker. Signals in opposite
  phase on L and R cancel.
- YIN every 60 ms (65 ms window, 28 to 1350 Hz), lags spread over the blocks. Interpolated dip, then refined on
  multiples of the period. Frames with RMS below the threshold skip the analysis.
- Tests: `tests/run.sh` (built binary, synthetic plucked strings).

## State & presets — AGREED
None.

## Zynthian widget — implemented, not tested on a zynthian
Half height: degree name of the selected scale with its octave, cents, needle (clamped at ±50 cents) and the
scale name. Green within 3 cents, orange within 15, red beyond (theme colours).

## Native GUI (X11, Cairo) — TODO
Still the generated generic GUI (a bar per control and output).

## Help page — implemented, not tested on a zynthian

## Open questions
- Root/key for the just and historical temperaments (fixed to C now)?
- Native GUI: keep the generic one or draw the tuner?
- GitHub: https://github.com/riban-bw/zyntuner · release v0.1.0 · 0.2.0 (mute, scales) not released
- Package: zynthian-packages PR: (pending)
