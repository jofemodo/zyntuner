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
# Copyright (C) 2026 riban
# License: GPL-3.0-or-later
# ******************************************************************************

import tkinter

from zyngui import zynthian_gui_config
from zyngui import zynthian_widget_base

NOTE_NAMES = ('C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B')
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
        self.cents_text = canvas.create_text(0, 0, text="", fill=zynthian_gui_config.color_panel_tx)
        self.last = None  # Last drawn (note, cents), () = no pitch

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
        canvas.coords(self.cents_text, int(self.width * 0.82), int(self.height * 0.72))
        canvas.itemconfigure(self.cents_text, font=(family, max(8, int(self.height * 0.1))))
        self.last = None  # Redraw with the new geometry

    def refresh_gui(self):
        # Called every ~32 ms while shown: only draw when something visible changed
        if not self.monitors or self.width <= 1:
            return
        if self.monitors.get("valid", 0) > 0.5:
            data = (int(round(self.monitors.get("note", 0))), round(self.monitors.get("cents", 0.0), 1))
        else:
            data = ()
        if data == self.last:
            return
        self.last = data
        canvas = self.widget_canvas
        if not data:
            canvas.itemconfigure(self.needle, state=tkinter.HIDDEN)
            canvas.itemconfigure(self.note_text, text="--", fill=zynthian_gui_config.color_tx_off)
            canvas.itemconfigure(self.cents_text, text="")
            return
        note, cents = data
        if abs(cents) <= IN_TUNE_CENTS:
            color = zynthian_gui_config.color_hl
        elif abs(cents) <= CLOSE_CENTS:
            color = zynthian_gui_config.color_warn
        else:
            color = zynthian_gui_config.color_error
        x = self.x0 + cents * self.cent_dx
        canvas.coords(self.needle, x - self.needle_w, self.y_top, x + self.needle_w, self.y_bottom)
        canvas.itemconfigure(self.needle, state=tkinter.NORMAL, fill=color)
        canvas.itemconfigure(self.note_text, text=f"{NOTE_NAMES[note % 12]}{note // 12 - 1}", fill=color)
        canvas.itemconfigure(self.cents_text, text=f"{cents:+.1f} ct")
