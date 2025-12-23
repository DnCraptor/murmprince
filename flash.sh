#!/usr/bin/env bash
set -euo pipefail

# Flash murmprince using picotool.
# Usage:
#   ./flash.sh                 # flashes ./build-make/murmprince.elf (falls back to ./build/murmprince.elf)
#   ./flash.sh path/to/app.elf # flashes a specific ELF

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -n "${1:-}" ]]; then
  ELF_PATH="$1"
else
  if [[ -f "$ROOT_DIR/build-make/murmprince.elf" ]]; then
    ELF_PATH="$ROOT_DIR/build-make/murmprince.elf"
  else
    ELF_PATH="$ROOT_DIR/build/murmprince.elf"
  fi
fi

if ! command -v picotool >/dev/null 2>&1; then
  echo "ERROR: picotool not found in PATH" >&2
  exit 127
fi

if [[ ! -f "$ELF_PATH" ]]; then
  echo "ERROR: ELF not found: $ELF_PATH" >&2
  echo "Run ./build.sh first." >&2
  exit 2
fi

picotool load "$ELF_PATH" -f

# Some RP2350 builds reboot fast enough that picotool reports a reboot error
# even though the flash completed successfully.
picotool reboot -f || true

echo "Flashed: $ELF_PATH"
