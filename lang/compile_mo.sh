#!/bin/sh

set -eu

if [ ! -d lang/po ]; then
    if [ -d ../lang/po ]; then
        cd ..
    else
        echo "Error: Could not find lang/po subdirectory." >&2
        exit 1
    fi
fi

if [ -z "${LOCALE_DIR:-}" ]; then
    LOCALE_DIR="lang/mo"
fi

if [ "$#" -gt 0 ] && [ "$1" != "all" ]; then
    languages="$*"
else
    languages=""
    for file in lang/po/*.po lang/po/*.po.gz; do
        [ -e "$file" ] || continue
        language=$(basename "$file")
        language=${language%.po.gz}
        language=${language%.po}
        languages="$languages $language"
    done
fi

for language in $languages; do
    po_file="lang/po/${language}.po"
    if [ ! -f "$po_file" ]; then
        po_file="lang/po/${language}.po.gz"
    fi
    if [ ! -f "$po_file" ]; then
        echo "Missing translation catalog: $po_file" >&2
        exit 1
    fi
    mkdir -p "$LOCALE_DIR/$language/LC_MESSAGES"
    python3 lang/compile_mo.py "$po_file" "$LOCALE_DIR/$language/LC_MESSAGES/cataclysm-dda.mo"
done
