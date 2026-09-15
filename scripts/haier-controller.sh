#!/usr/bin/env bash
# Repeatable project workflow. Build is read-only; flash is explicit.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_PATH="apps/haier_controller"
TEST_APP_PATH="apps/testScreen"
PORT="${HAIER_PORT:-}"

usage() {
  cat >&2 <<'EOF'
Usage: scripts/haier-controller.sh <command> [port]

Commands:
  build          Build firmware and SPIFFS storage image
  flash [port]   Flash firmware and storage to the board
  build-flash    Build, then flash to the board
  monitor [port] Open the ESP-IDF serial monitor
  check          Validate shared model and generated build artifacts
  preview        Serve the browser preview on port 4173
  env            Check the ESP-IDF environment
  test-build     Build the clean Hello World test screen
  test-flash [port] Flash the clean Hello World test screen
  test-build-flash [port] Build and flash the clean Hello World test screen
  test-monitor [port] Monitor the clean Hello World test screen
EOF
  exit 1
}

detect_port() {
  mapfile -t ports < <(find /dev -maxdepth 1 -type c \( -name 'ttyACM*' -o -name 'ttyUSB*' \) -printf '%p\n' | sort)
  if [[ "${#ports[@]}" -eq 1 ]]; then
    printf '%s\n' "${ports[0]}"
    return
  fi
  if [[ "${#ports[@]}" -eq 0 ]]; then
    echo "No /dev/ttyACM* or /dev/ttyUSB* device found." >&2
  else
    echo "Multiple serial devices found:" >&2
    printf '  %s\n' "${ports[@]}" >&2
  fi
  echo "Pass the port explicitly or set HAIER_PORT." >&2
  exit 1
}

flash() {
  local port_value="${1:-$PORT}"
  [[ -n "$port_value" ]] || port_value="$(detect_port)"
  exec bash "$SCRIPT_DIR/flash-device.sh" haier "$port_value"
}

test_flash() {
  local port_value="${1:-$PORT}"
  [[ -n "$port_value" ]] || port_value="$(detect_port)"
  exec bash "$SCRIPT_DIR/flash-device.sh" testScreen "$port_value"
}

command_name="${1:-}"
shift || true
case "$command_name" in
  build)
    exec bash "$SCRIPT_DIR/idf-task.sh" "$APP_PATH" build
    ;;
  flash)
    flash "${1:-}"
    ;;
  build-flash)
    bash "$SCRIPT_DIR/idf-task.sh" "$APP_PATH" build
    flash "${1:-}"
    ;;
  test-build)
    exec bash "$SCRIPT_DIR/idf-task.sh" "$TEST_APP_PATH" build
    ;;
  test-flash)
    test_flash "${1:-}"
    ;;
  test-build-flash)
    bash "$SCRIPT_DIR/idf-task.sh" "$TEST_APP_PATH" build
    test_flash "${1:-}"
    ;;
  test-monitor)
    port_value="${1:-$PORT}"
    [[ -n "$port_value" ]] || port_value="$(detect_port)"
    exec bash "$SCRIPT_DIR/idf-task.sh" "$TEST_APP_PATH" monitor -p "$port_value"
    ;;
  monitor)
    port_value="${1:-$PORT}"
    [[ -n "$port_value" ]] || port_value="$(detect_port)"
    exec bash "$SCRIPT_DIR/idf-task.sh" "$APP_PATH" monitor -p "$port_value"
    ;;
  check)
    cd "$REPO_ROOT"
    jq -e '.settings and (.tiles | length == 9)' common/dashboard/dashboard_model.json >/dev/null
    test -f apps/haier_controller/build/haier_controller.bin
    test -f apps/haier_controller/build/storage.bin
    echo "Shared model and firmware/storage artifacts are valid."
    ;;
  preview)
    cd "$REPO_ROOT"
    exec python3 -m http.server "${1:-4173}"
    ;;
  env)
    exec bash "$SCRIPT_DIR/setup-env.sh"
    ;;
  *)
    usage
    ;;
esac
