#!/usr/bin/env bash
set -euo pipefail

task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
task_build="$(mktemp -d)"
trap 'rm -rf -- "$task_build"' EXIT

cat > "$task_build/main.cpp" <<'CPP'
#define CATCH_CONFIG_MAIN
#include "cata_catch.h"
CPP

"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -pedantic \
    -I"$task_root/src" -I"$task_root/tests" -isystem "$task_root/src/third-party" \
    "$task_build/main.cpp" "$task_root/tests/a11y_context_test.cpp" \
    "$task_root/src/a11y_context.cpp" -o "$task_build/a11y_core_test"
"$task_build/a11y_core_test" '[a11y]'
