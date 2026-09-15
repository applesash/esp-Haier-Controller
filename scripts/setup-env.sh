#!/usr/bin/env bash
# Check the local ESP-IDF environment used by this project.
# Dependency installation remains explicit; this script never runs sudo.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
IDF_PATH_VALUE="${IDF_PATH:-$HOME/esp/esp-idf}"

fail() {
  echo "setup-env: $*" >&2
  exit 1
}

command -v python3 >/dev/null 2>&1 || fail "python3 is required"
command -v cmake >/dev/null 2>&1 || fail "cmake is required"
command -v ninja >/dev/null 2>&1 || fail "ninja is required"
[[ -f "$IDF_PATH_VALUE/export.sh" ]] || fail "ESP-IDF export.sh not found at $IDF_PATH_VALUE"

export IDF_PATH="$IDF_PATH_VALUE"
# shellcheck disable=SC1090
source "$IDF_PATH/export.sh" >/dev/null

cd "$REPO_ROOT"
printf 'Project: %s\n' "$REPO_ROOT"
printf 'ESP-IDF: %s\n' "$IDF_PATH"
printf 'Target: esp32s3\n'
printf 'Python: %s\n' "$(python3 --version 2>&1)"
printf 'idf.py: %s\n' "$(idf.py --version 2>&1 | head -1)"
printf 'Environment is ready.\n'
