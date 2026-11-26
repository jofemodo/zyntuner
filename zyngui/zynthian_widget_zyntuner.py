# -*- coding: utf-8 -*-
# ******************************************************************************
# ZYNTHIAN PROJECT: Zynthian widget for "zyntuner"
#
# Plugin: <https://zynthian.org/plugins/zyntuner>
# Declared as a zyngui:widget LV2 UI (see the bundle's manifest.ttl).
#
# Data: LV2 output control ports ("monitors"), polled ~30 times/s while shown:
#   self.monitors[symbol] -> float
# Controls: self.processor.controllers_dict[symbol] (zctrl: .value, .set_value(v, True), .nudge(n))
#   The controls of the page being shown: self.zyngui_control.zcontrollers
#
# Note names per scale: zyntuner_scales.py, next to this file (also the DSP's scale table)
#
# Copyright (C) 2026 riban
# License: GPL-3.0-or-later
# ******************************************************************************

import importlib.util
import os
import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base



def _load_scales():
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "zyntuner_scales.py")
    spec = importlib.util.spec_from_file_location("zyntuner_scales", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


_SCALES = _load_scales().SCALES
SCALE_LABELS = [s[0] for s in _SCALES]
SCALE_NAMES = [[name for _, name in s[3]] for s in _SCALES]
IN_TUNE_CENTS = 3.0
CLOSE_CENTS = 15.0


class zynthian_widget_zyntuner(zynthian_widget_base.zynthian_widget_base):

    def __init__(self, parent):
        super().__init__(parent)
        # Half-height widget: the list of pages stays visible above it
        self.rows = max(1, self.rows // 2)
        self.widget_canvas = tkinter.Canvas(self, highlightthickness=0, relief='flat',
                                            bg=zynthian_gui_config.color_bg)
        self.widget_canvas.grid(sticky='news')

        # Create all the canvas items once: refresh_gui() only moves/changes them
        canvas = self.widget_canvas
        self.target = canvas.create_rectangle(0, 0, 0, 0, width=0, fill=zynthian_gui_config.color_panel_bg)
        self.ticks = [canvas.create_line(0, 0, 0, 0, fill=zynthian_gui_config.color_tx_off)
                      for _ in range(-50, 51, 10)]
        self.needle = canvas.create_rectangle(0, 0, 0, 0, width=0, state=tkinter.HIDDEN)
        self.note_text = canvas.create_text(0, 0, text="--", fill=zynthian_gui_config.color_tx_off)
        self.octave_text = canvas.create_text(0, 0, text="", anchor=tkinter.W)
        self.cents_text = canvas.create_text(0, 0, text="", fill=zynthian_gui_config.color_panel_tx)
        self.scale_text = canvas.create_text(0, 0, text="", fill=zynthian_gui_config.color_tx_off)
        self.last = None  # Last drawn (scale, degree, octave, cents), (scale,) = no pitch
        self.last_scale = None

    def on_size(self, event):
        if not super().on_size(event):
            return
        canvas = self.widget_canvas
        self.x0 = self.width / 2
        self.cent_dx = self.width * 0.9 / 100
        self.needle_w = max(2, self.width // 100)
        self.y_top = int(self.height * 0.08)
        self.y_bottom = int(self.height * 0.42)
        dx = self.cent_dx * IN_TUNE_CENTS
        canvas.coords(self.target, self.x0 - dx, self.y_top, self.x0 + dx, self.y_bottom)
        for item, cents in zip(self.ticks, range(-50, 51, 10)):
            x = self.x0 + cents * self.cent_dx
            y = self.y_top if cents == 0 else (self.y_top + self.y_bottom) // 2
            canvas.coords(item, x, y, x, self.y_bottom)
        family = zynthian_gui_config.font_family
        canvas.coords(self.note_text, self.x0, int(self.height * 0.72))
        canvas.itemconfigure(self.note_text, font=(family, max(10, int(self.height * 0.25))))
        canvas.coords(self.cents_text, int(self.width * 0.86), int(self.height * 0.72))
        small = (family, max(8, int(self.height * 0.1)))
        canvas.itemconfigure(self.cents_text, font=small)
        canvas.itemconfigure(self.octave_text, font=small)
        canvas.coords(self.scale_text, int(self.width * 0.14), int(self.height * 0.72))
        canvas.itemconfigure(self.scale_text, font=small, width=int(self.width * 0.26))
        self.last = None
        self.last_scale = None  # Redraw with the new geometry

    def get_scale(self):
        try:
            scale = int(self.processor.controllers_dict["scale"].value)
        except (AttributeError, KeyError, TypeError, ValueError):
            scale = 0
        return scale if 0 <= scale < len(SCALE_NAMES) else 0

    def refresh_gui(self):
        # Called every ~32 ms while shown: only draw when something visible changed
        if not self.monitors or self.width <= 1:
            return
        scale = self.get_scale()
        if self.monitors.get("valid", 0) > 0.5:
            data = (scale, int(round(self.monitors.get("degree", 0))), int(round(self.monitors.get("octave", 4))),
                    round(self.monitors.get("cents", 0.0), 1))
        else:
            data = (scale,)
        if data == self.last:
            return
        self.last = data
        canvas = self.widget_canvas
        if scale != self.last_scale:
            self.last_scale = scale
            canvas.itemconfigure(self.scale_text, text=SCALE_LABELS[scale])
        if len(data) == 1:
            canvas.itemconfigure(self.needle, state=tkinter.HIDDEN)
            canvas.itemconfigure(self.note_text, text="--", fill=zynthian_gui_config.color_tx_off)
            canvas.itemconfigure(self.octave_text, text="")
            canvas.itemconfigure(self.cents_text, text="")
            return
        _, degree, octave, cents = data
        names = SCALE_NAMES[scale]
        name = names[degree] if 0 <= degree < len(names) else "?"
        if abs(cents) <= IN_TUNE_CENTS:
            color = zynthian_gui_config.color_hl
        elif abs(cents) <= CLOSE_CENTS:
            color = zynthian_gui_config.color_warn
        else:
            color = zynthian_gui_config.color_error
        # The needle shows +/-50 cents: beyond (scales with wide steps) it stays at the edge
        x = self.x0 + max(-50.0, min(50.0, cents)) * self.cent_dx
        canvas.coords(self.needle, x - self.needle_w, self.y_top, x + self.needle_w, self.y_bottom)
        canvas.itemconfigure(self.needle, state=tkinter.NORMAL, fill=color)
        canvas.itemconfigure(self.note_text, text=name, fill=color)
        bbox = canvas.bbox(self.note_text)
        if bbox:
            canvas.coords(self.octave_text, bbox[2] + 2, int(self.height * 0.80))
        canvas.itemconfigure(self.octave_text, text=str(octave), fill=color)
        canvas.itemconfigure(self.cents_text, text=f"{cents:+.1f} ct")
