PO_DIR := ./po
MO_DIR := ./mo
PO_FILES := $(notdir $(wildcard $(PO_DIR)/*.po $(PO_DIR)/*.po.gz))
PO := $(sort $(patsubst %.po.gz,%,$(filter %.po.gz,$(PO_FILES))) $(patsubst %.po,%,$(filter %.po,$(PO_FILES))))

ifeq ($(LANGUAGES),)
  # Use every catalog available in the source tree.
else ifneq ($(LANGUAGES),all)
  PO := $(LANGUAGES)
endif

MO := $(patsubst %,$(MO_DIR)/%/LC_MESSAGES/cataclysm-dda.mo,$(PO))

all: $(MO)

$(MO_DIR)/%/LC_MESSAGES/cataclysm-dda.mo: $(PO_DIR)/%.po
	mkdir -p $$(dirname $@)
	msgfmt -f -o $@ $<

$(MO_DIR)/%/LC_MESSAGES/cataclysm-dda.mo: $(PO_DIR)/%.po.gz ../lang/compile_mo.py
	mkdir -p $$(dirname $@)
	python3 ../lang/compile_mo.py $< $@

list_po:
	@echo $(PO)

list_mo:
	@echo $(MO)

install: $(MO)
ifdef LOCALE_DIR
	mkdir -p $(LOCALE_DIR)
	cp -R --no-preserve=ownership $(MO_DIR)/* $(LOCALE_DIR)
endif

clean:
	find mo -type d -mindepth 1 -maxdepth 1 -exec rm -rf {} +

.PHONY: all list_po list_mo clean
