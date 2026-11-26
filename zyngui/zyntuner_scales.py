#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ******************************************************************************
# zyntuner: scales / tunings. Single source of truth for the DSP and the widget.
#
# Imported by the zynthian widget (note names) and run at build time to generate
# plugins/zyntuner/Scales.hpp (pitches, used by the DSP):
#   python3 zyngui/zyntuner_scales.py > plugins/zyntuner/Scales.hpp
# The order of SCALES is the value of the "scale" control: append new scales at the end
# (saved presets and snapshots store the index).
#
# Each scale: (label, period in cents, anchor, [(cents, name), ...])
# - cents: pitch of each degree above the root (degree 0 = 0 cents), ascending, < period.
# - anchor: "A" = the degree named A in octave 4 sounds at the A4 reference (western tunings);
#   "C" = the root sounds at the equal tempered C4 of the reference (ref * 2^(-9/12)).
#
# Copyright (C) 2026 riban
# License: GPL-3.0-or-later
# ******************************************************************************

import math


def ratio(r):
    """Cents of a ratio given as 'p/q' or a number"""
    if isinstance(r, str):
        p, q = r.split("/")
        r = int(p) / int(q)
    return 1200.0 * math.log2(r)


def edo(n, names, period=1200.0):
    """n equal divisions of the period"""
    assert len(names) == n
    return [(period * i / n, names[i]) for i in range(n)]


def from_cents(cents, names):
    assert len(cents) == len(names)
    return list(zip(cents, names))


def from_ratios(ratios, names=None):
    return [(ratio(r), names[i] if names else r) for i, r in enumerate(ratios)]


NAMES_12 = ("C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B")


def names_53():
    """53-EDO: naturals, sharps and flats on the Pythagorean chain, the other steps as +/- commas"""
    naturals = {"C": 0, "D": 9, "E": 18, "F": 22, "G": 31, "A": 40, "B": 49}
    named = {}
    for n, s in naturals.items():
        named[s] = n
    for n, s in naturals.items():
        named.setdefault((s + 5) % 53, n + "#")
        named.setdefault((s - 5) % 53, n + "b")
    names = []
    for step in range(53):
        if step in named:
            names.append(named[step])
            continue
        best = None
        for s, n in named.items():
            for d in (step - s, step - s - 53, step - s + 53):
                # Prefer the smallest offset, then naturals, then "+" offsets
                key = (abs(d), len(n), d < 0)
                if best is None or key < best[0]:
                    best = (key, f"{n}{'+' if d > 0 else '-'}{abs(d)}")
        names.append(best[1])
    return names


SCALES = [
    ("12-TET", 1200.0, "A", edo(12, NAMES_12)),
    ("Just (C)", 1200.0, "A", from_ratios(
        ("1/1", "16/15", "9/8", "6/5", "5/4", "4/3", "45/32", "3/2", "8/5", "5/3", "9/5", "15/8"), NAMES_12)),
    ("Pythagorean (C)", 1200.0, "A", from_ratios(
        ("1/1", "2187/2048", "9/8", "32/27", "81/64", "4/3", "729/512", "3/2", "6561/4096", "27/16", "16/9",
         "243/128"), NAMES_12)),
    ("Meantone 1/4", 1200.0, "A", from_cents(
        (0.0, 76.05, 193.16, 310.26, 386.31, 503.42, 579.47, 696.58, 772.63, 889.74, 1006.84, 1082.89), NAMES_12)),
    ("Werckmeister III", 1200.0, "A", from_cents(
        (0.0, 90.22, 192.18, 294.13, 390.22, 498.04, 588.27, 696.09, 792.18, 888.27, 996.09, 1092.18), NAMES_12)),
    ("Kirnberger III", 1200.0, "A", from_cents(
        (0.0, 90.22, 193.16, 294.13, 386.31, 498.04, 590.22, 696.58, 792.18, 889.74, 996.09, 1088.27), NAMES_12)),
    ("Vallotti", 1200.0, "A", from_cents(
        (0.0, 94.13, 196.09, 298.04, 392.18, 501.96, 592.18, 698.04, 796.09, 894.13, 1000.0, 1090.22), NAMES_12)),
    ("19-TET", 1200.0, "A", edo(19, (
        "C", "C#", "Db", "D", "D#", "Eb", "E", "E#", "F", "F#", "Gb", "G", "G#", "Ab", "A", "A#", "Bb", "B", "B#"))),
    ("24-TET", 1200.0, "A", edo(24, (
        "C", "C+", "C#", "C#+", "D", "D+", "Eb", "Eb+", "E", "E+", "F", "F+",
        "F#", "F#+", "G", "G+", "G#", "G#+", "A", "A+", "Bb", "Bb+", "B", "B+"))),
    ("31-TET", 1200.0, "A", edo(31, (
        "C", "C+", "C#", "Db", "D-", "D", "D+", "D#", "Eb", "E-", "E", "E+", "F-", "F", "F+", "F#",
        "Gb", "G-", "G", "G+", "G#", "Ab", "A-", "A", "A+", "A#", "Bb", "B-", "B", "B+", "C-"))),
    ("53-TET", 1200.0, "A", edo(53, names_53())),
    ("22 Shruti", 1200.0, "C", from_ratios(
        ("1/1", "256/243", "16/15", "10/9", "9/8", "32/27", "6/5", "5/4", "81/64", "4/3", "27/20", "45/32",
         "729/512", "3/2", "128/81", "8/5", "5/3", "27/16", "16/9", "9/5", "15/8", "243/128"),
        ("Sa", "Re1", "Re2", "Re3", "Re4", "Ga1", "Ga2", "Ga3", "Ga4", "Ma1", "Ma2", "Ma3", "Ma4", "Pa",
         "Dha1", "Dha2", "Dha3", "Dha4", "Ni1", "Ni2", "Ni3", "Ni4"))),
    ("Slendro", 1200.0, "C", edo(5, ("1", "2", "3", "5", "6"))),
    ("Pelog", 1200.0, "C", from_cents(
        (0.0, 120.0, 258.0, 539.0, 675.0, 785.0, 943.0), ("1", "2", "3", "4", "5", "6", "7"))),
    ("Thai 7-TET", 1200.0, "C", edo(7, ("1", "2", "3", "4", "5", "6", "7"))),
    ("Harmonics 8-15", 1200.0, "C", from_ratios(
        ("8/8", "9/8", "10/8", "11/8", "12/8", "13/8", "14/8", "15/8"),
        ("8", "9", "10", "11", "12", "13", "14", "15"))),
    ("Partch 43", 1200.0, "C", from_ratios((
        "1/1", "81/80", "33/32", "21/20", "16/15", "12/11", "11/10", "10/9", "9/8", "8/7", "7/6", "32/27",
        "6/5", "11/9", "5/4", "14/11", "9/7", "21/16", "4/3", "27/20", "11/8", "7/5", "10/7", "16/11",
        "40/27", "3/2", "32/21", "14/9", "11/7", "8/5", "18/11", "5/3", "27/16", "12/7", "7/4", "16/9",
        "9/5", "20/11", "11/6", "15/8", "40/21", "64/33", "160/81"))),
    ("Bohlen-Pierce", ratio(3), "C", edo(13, (
        "C", "Db", "D", "E", "F", "Gb", "G", "H", "Jb", "J", "A", "Bb", "B"), ratio(3))),
]


def check():
    for label, period, anchor, degrees in SCALES:
        cents = [c for c, _ in degrees]
        assert cents[0] == 0.0 and all(a < b for a, b in zip(cents, cents[1:])) and cents[-1] < period, label
        assert anchor == "C" or "A" in [n for _, n in degrees], label
        assert len(set(n for _, n in degrees)) == len(degrees), label
    assert len(SCALES[16][3]) == 43


def anchor_cents(scale):
    """Cents of the anchor above the root: the root is at ref * 2^(-anchor_cents / 1200)"""
    label, period, anchor, degrees = scale
    if anchor == "A":
        return next(c for c, n in degrees if n == "A")
    return 900.0


def header():
    check()
    out = ["// Generated by zyngui/zyntuner_scales.py: don't edit, edit the Python table and run make.",
           "#pragma once", "", "#include <cstdint>", "",
           "struct ScaleDef {", "    const char* label;", "    double period;  // cents",
           "    double anchor;  // cents of the anchor (A4 = reference) above the root",
           "    uint32_t count;", "    const double* cents;",
           "    const char* const* names;  // degree names (native GUI)", "};", ""]
    for i, (label, period, anchor, degrees) in enumerate(SCALES):
        vals = ", ".join(f"{c:.6f}" for c, _ in degrees)
        out.append(f"static const double kScale{i}Cents[] = {{ {vals} }};")
        names = ", ".join(f'"{n}"' for _, n in degrees)
        out.append(f"static const char* const kScale{i}Names[] = {{ {names} }};")
    out += ["", f"static const uint32_t kScaleCount = {len(SCALES)};", "",
            "static const ScaleDef kScales[kScaleCount] = {"]
    for i, s in enumerate(SCALES):
        out.append(f'    {{ "{s[0]}", {s[1]:.6f}, {anchor_cents(s):.6f}, {len(s[3])}, kScale{i}Cents, '
                   f'kScale{i}Names }},')
    out += ["};", "", "static const char* const kScaleLabels[kScaleCount] = {"]
    out += [f'    "{s[0]}",' for s in SCALES]
    out += ["};", ""]
    return "\n".join(out)


if __name__ == "__main__":
    print(header(), end="")
