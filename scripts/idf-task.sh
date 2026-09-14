#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <app-path> [idf.py args...]" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_PATH="$1"
shift || true

resolve_idf_path() {
  if [ -n "${IDF_PATH:-}" ] && [ -f "$IDF_PATH/export.sh" ]; then
    printf '%s\n' "$IDF_PATH"
    return
  fi

  local config_path="$REPO_ROOT/$APP_PATH/build/config.env"
  if [ -f "$config_path" ]; then
    local candidate
    candidate="$(sed -n 's/.*"IDF_PATH":[[:space:]]*"\([^"]*\)".*/\1/p' "$config_path" | head -n 1)"
    if [ -n "$candidate" ] && [ -f "$candidate/export.sh" ]; then
      printf '%s\n' "$candidate"
      return
    fi
  fi

  for candidate in "$HOME/esp/esp-idf" "/opt/esp/idf"; do
    if [ -f "$candidate/export.sh" ]; then
      printf '%s\n' "$candidate"
      return
    fi
  done

  echo "Unable to find ESP-IDF. Set IDF_PATH or run scripts/setup-env.sh." >&2
  exit 1
}

resolve_idf_python_env_path() {
  if [ -n "${IDF_PYTHON_ENV_PATH:-}" ] && [ -d "$IDF_PYTHON_ENV_PATH" ]; then
    printf '%s\n' "$IDF_PYTHON_ENV_PATH"
    return
  fi

  for root in "$HOME/.espressif/python_env" "$HOME/esp/python_env"; do
    if [ -d "$root" ]; then
      local match
      match="$(find "$root" -maxdepth 1 -mindepth 1 -type d -name 'idf*_py*_env' | sort | tail -n 1)"
      if [ -n "$match" ]; then
        printf '%s\n' "$match"
        return
      fi
    fi
  done
}

export IDF_PATH="$(resolve_idf_path)"
IDF_PYTHON_ENV_PATH_VALUE="$(resolve_idf_python_env_path || true)"
if [ -n "$IDF_PYTHON_ENV_PATH_VALUE" ]; then
  export IDF_PYTHON_ENV_PATH="$IDF_PYTHON_ENV_PATH_VALUE"
fi

# shellcheck disable=SC1090
source "$IDF_PATH/export.sh"

cd "$REPO_ROOT/$APP_PATH"
if [ "$#" -eq 0 ]; then
  set -- build
fi

idf.py "$@"
