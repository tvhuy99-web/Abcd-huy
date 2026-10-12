MO_DIR := ./mo

all:
	cd .. && LOCALE_DIR="$(abspath $(MO_DIR))" sh lang/compile_mo.sh

list_po list_mo:
	@:

install: all
	@echo "English-only build: no translation catalogs to install."

clean:
	@if [ -d "$(MO_DIR)" ]; then find "$(MO_DIR)" -type f -name '*.mo' -delete; fi

.PHONY: all list_po list_mo install clean
