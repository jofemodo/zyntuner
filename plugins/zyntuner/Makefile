#!/usr/bin/make -f
# zyntuner - plugin Makefile (DPF)

NAME = zyntuner

FILES_DSP = Plugin.cpp
FILES_UI = UI.cpp

# Native GUI with the Cairo backend (X11UI)
UI_TYPE = cairo

include ../../dpf/Makefile.plugins.mk

# LV2 with DSP and UI in separate binaries (the DSP never loads GUI libraries)
TARGETS = lv2_sep

all: $(TARGETS)
