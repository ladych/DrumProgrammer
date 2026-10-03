#!/usr/bin/env bash
# Runs clang-tidy on all of our sources in src/ (E-08).
# Usage: tools/run-clang-tidy.sh <build-dir-with-compile_commands.json>
set -euo pipefail

build_dir="${1:?build directory with compile_commands.json required}"
repo_root="$(cd "$(dirname "$0")/.." && pwd)"

mapfile -t sources < <(cd "$repo_root" && find src -name '*.cpp' | sort)
cd "$repo_root"
printf '%s\n' "${sources[@]}" | xargs -P "$(nproc)" -n 1 clang-tidy --quiet -p "$build_dir"
