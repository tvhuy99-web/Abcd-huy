#!/usr/bin/env bash
set -euo pipefail

task_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
task_build="$(mktemp -d)"
trap 'rm -rf -- "$task_build"' EXIT

if command -v javac >/dev/null 2>&1; then
    task_compiler=(javac)
else
    task_compiler=(java --module jdk.compiler/com.sun.tools.javac.Main)
fi
"${task_compiler[@]}" -Xlint:all -Werror -d "$task_build" \
    "$task_root/android/app/src/main/java/com/cleverraven/cataclysmdda/SpeechSession.java" \
    "$task_root/tools/accessibility/java/com/cleverraven/cataclysmdda/SpeechSessionTest.java"
java -cp "$task_build" com.cleverraven.cataclysmdda.SpeechSessionTest

mapfile -t task_fakes < <(rg --files "$task_root/tools/accessibility/fakes" -g '*.java' | sort)
"${task_compiler[@]}" -Xlint:all -Werror -d "$task_build" \
    "${task_fakes[@]}" \
    "$task_root/android/app/src/main/java/com/cleverraven/cataclysmdda/AndroidSpeech.java" \
    "$task_root/android/app/src/main/java/com/cleverraven/cataclysmdda/SpeechSession.java" \
    "$task_root/tools/accessibility/java/com/cleverraven/cataclysmdda/AndroidSpeechTest.java"
java -cp "$task_build" com.cleverraven.cataclysmdda.AndroidSpeechTest
