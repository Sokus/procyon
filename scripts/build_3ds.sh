#!/bin/sh

export DEVKITARM=/opt/devkitpro/devkitARM;
export DEVKITPPC=/opt/devkitpro/devkitPPC;
export DEVKITPRO=/opt/devkitpro;

BUILD_TYPE=${1:-Release}
SCRIPT_DIRECTORY=$(dirname "$0")

cmake -DCMAKE_TOOLCHAIN_FILE="$SCRIPT_DIRECTORY/../cmake/DevkitArm3DS.cmake" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -B 3ds_build -S . || exit 1
make --directory="$SCRIPT_DIRECTORY/../3ds_build" --no-print-directory || exit 1
