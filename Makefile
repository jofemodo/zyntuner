#!/usr/bin/make -f
# zyntuner - DPF based LV2 plugin with native X11 GUI (Cairo) and zynthian widget

include dpf/Makefile.base.mk

PLUGIN = zyntuner
PREFIX ?= /usr/local
LV2_DIR ?= $(PREFIX)/lib/lv2

all: dgl plugins gen zyngui

dgl:
ifeq ($(HAVE_CAIRO),true)
	$(MAKE) -C dpf/dgl cairo
endif

plugins: dgl
	$(MAKE) all -C plugins/$(PLUGIN)

ifneq ($(CROSS_COMPILING),true)
gen: plugins dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen:
endif

# Add the zynthian widget (zyngui:widget), help page and port extras to the generated bundle
zyngui: gen
	python3 tools/zyngui_bundle.py bin/$(PLUGIN).lv2

install: all
	install -d $(DESTDIR)$(LV2_DIR)
	cp -r bin/$(PLUGIN).lv2 $(DESTDIR)$(LV2_DIR)/

clean:
	$(MAKE) clean -C dpf/dgl
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(MAKE) clean -C plugins/$(PLUGIN)
	rm -rf bin build

.PHONY: all dgl plugins gen zyngui install clean
