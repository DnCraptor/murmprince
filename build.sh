#!/usr/bin/env bash
set -euo pipefail

# Build murmprince using CMake + Make.
# Usage:
#   ./build.sh            # configure (if needed) + build in ./build-make
#   ./build.sh clean      # deletes ./build-make then rebuilds

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build-make"

if [[ "${1:-}" == "clean" ]]; then
  rm -rf "$BUILD_DIR"
fi

cmake_args=()

# Optional overrides for quick A/B testing without editing CMakeLists.txt.
# Examples:
#   CPU_SPEED=252 ./build.sh clean
#   CPU_SPEED=504 PSRAM_SPEED=133 ./build.sh
if [[ -n "${CPU_SPEED:-}" ]]; then
  cmake_args+=("-DCPU_SPEED=${CPU_SPEED}")
fi
if [[ -n "${PSRAM_SPEED:-}" ]]; then
  cmake_args+=("-DPSRAM_SPEED=${PSRAM_SPEED}")
fi
if [[ -n "${BOARD_VARIANT:-}" ]]; then
  cmake_args+=("-DBOARD_VARIANT=${BOARD_VARIANT}")
fi

# Diagnostics toggles
if [[ -n "${RP2350_FORCE_TEST_PATTERN:-}" ]]; then
  cmake_args+=("-DRP2350_FORCE_TEST_PATTERN=${RP2350_FORCE_TEST_PATTERN}")
fi
if [[ -n "${RP2350_DUMP_FIRST_FRAME_BYTES:-}" ]]; then
  cmake_args+=("-DRP2350_DUMP_FIRST_FRAME_BYTES=${RP2350_DUMP_FIRST_FRAME_BYTES}")
fi

if [[ -n "${RP2350_DEBUG_INDEX_BAR:-}" ]]; then
  cmake_args+=("-DRP2350_DEBUG_INDEX_BAR=${RP2350_DEBUG_INDEX_BAR}")
fi
if [[ -n "${RP2350_BOOT_TEST_PATTERN:-}" ]]; then
  cmake_args+=("-DRP2350_BOOT_TEST_PATTERN=${RP2350_BOOT_TEST_PATTERN}")
fi
if [[ -n "${RP2350_BOOT_TEST_PATTERN_HALT:-}" ]]; then
  cmake_args+=("-DRP2350_BOOT_TEST_PATTERN_HALT=${RP2350_BOOT_TEST_PATTERN_HALT}")
fi
if [[ -n "${RP2350_BOOT_TEST_PATTERN_MODE:-}" ]]; then
  cmake_args+=("-DRP2350_BOOT_TEST_PATTERN_MODE=${RP2350_BOOT_TEST_PATTERN_MODE}")
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "Unix Makefiles" ${cmake_args[@]+"${cmake_args[@]}"}
cmake --build "$BUILD_DIR" -- -j

echo "Built: $BUILD_DIR/murmprince.elf"
