#!/usr/bin/env bash
# Flash the Haier controller firmware to a connected board over USB in one step:
# auto-detects the serial port, fixes serial-port permissions if needed, then
# delegates to idf-task.sh. Avoids the manual port/permission diagnosis loop.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  echo "Usage: $0 haier [serial-port]" >&2
  exit 1
}

[ "$#" -ge 1 ] || usage
TARGET="$1"
case "$TARGET" in
  haier) APP_PATH="apps/haier_controller" ;;
  *)
    echo "Unknown target '$TARGET' (expected 'haier')" >&2
    usage
    ;;
esac
PORT="${2:-}"

detect_ports() {
  compgen -G "/dev/ttyACM*" 2>/dev/null || true
  compgen -G "/dev/ttyUSB*" 2>/dev/null || true
}

if [ -z "$PORT" ]; then
  mapfile -t PORTS < <(detect_ports)
  if [ "${#PORTS[@]}" -eq 0 ]; then
    echo "No serial device found (checked /dev/ttyACM* and /dev/ttyUSB*)." >&2
    echo "Plug in the $TARGET board over USB and retry." >&2
    exit 1
  elif [ "${#PORTS[@]}" -eq 1 ]; then
    PORT="${PORTS[0]}"
  else
    echo "Multiple serial devices found:" >&2
    printf '  %s\n' "${PORTS[@]}" >&2
    echo "Disconnect the other board, or specify one: $0 $TARGET <port>" >&2
    exit 1
  fi
fi

ensure_port_access() {
  local port="$1"
  if [ -r "$port" ] && [ -w "$port" ]; then
    return 0
  fi
  echo "No permission to access $port; requesting temporary access (sudo)..." >&2
  if ! command -v sudo >/dev/null 2>&1; then
    echo "sudo not available; cannot fix permissions on $port." >&2
    exit 1
  fi
  sudo chmod a+rw "$port"
  if command -v getent >/dev/null 2>&1 && getent group dialout >/dev/null 2>&1; then
    if ! id -nG "$USER" | tr ' ' '\n' | grep -qx dialout; then
      sudo usermod -aG dialout "$USER" >/dev/null 2>&1 || true
      echo "Added $USER to the 'dialout' group for future flashes (log out/in to apply; this flash uses the temporary chmod above)." >&2
    fi
  fi
}

ensure_port_access "$PORT"

echo "Flashing $TARGET to $PORT..."
exec bash "$SCRIPT_DIR/idf-task.sh" "$APP_PATH" flash -p "$PORT"
