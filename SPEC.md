# Zyntuner: specification

Living document: each decision made in the chat is written here before it's implemented.
Status of each section: TODO / AGREED / DONE (implemented and tested on a zynthian).

## Identity — AGREED
- Name: Zyntuner · symbol: `zyntuner` · URI: `https://zynthian.org/plugins/zyntuner`
- Maker: riban · license: GPL-3.0-or-later · LV2 category: lv2:AnalyserPlugin
- Purpose: Low CPU guitar tuner with stereo pass-through

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

## Outputs (LV2 output ports → widget monitors, ~30 Hz) — implemented, not tested on a zynthian

| Symbol | Name | Range | Unit | Meaning / how it's computed |
|---|---|---|---|---|
| `note` | MIDI note | 0 .. 127 |  | nearest MIDI note; keeps its last value when not valid |
| `cents` | Cents | -50 .. 50 |  | deviation from `note` at the A4 reference |
| `valid` | Pitch valid | 0 .. 1 |  |  1 = pitch detected, 0 = silence / no pitch / bypassed |

## DSP — implemented, not tested on a zynthian
- Audio: both channels copied unchanged, no latency. Bypass needs no crossfade: it only stops the detection.
- Detection on 0.5 * (L + R): 4th order 1.5 kHz low-pass, decimation to ~6 kHz, DC blocker. Signals in opposite
  phase on L and R cancel.
- YIN every 60 ms (65 ms window, 28 to 1350 Hz), lags spread over the blocks. Interpolated dip, then refined on
  multiples of the period. Frames with RMS below the threshold skip the analysis.
- Tests: `tests/run.sh` (built binary, synthetic plucked strings).

## State & presets — AGREED
None.

## Zynthian widget — implemented, not tested on a zynthian
Half height: note name, cents and needle. Green within 3 cents, orange within 15, red beyond (theme colours).

## Native GUI (X11, Cairo) — TODO
Still the generated generic GUI (a bar per control and output).

## Help page — implemented, not tested on a zynthian

## Open questions
- Fourth control on the page (soft rule 2: 3 is acceptable)?
- Native GUI: keep the generic one or draw the tuner?
- GitHub owner: `OWNER` is a placeholder in README, package and spec.json.
