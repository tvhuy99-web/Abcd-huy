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

# English is built into the source. Do not compile or ship translated catalogs.
catalog_dir=${LOCALE_DIR:-lang/mo}
if [ -d "$catalog_dir" ]; then
    find "$catalog_dir" -type f -name '*.mo' -delete
fi
echo "English-only build: no translation catalogs required."
