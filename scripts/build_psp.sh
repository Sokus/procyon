#!/bin/sh

SCRIPT_DIRECTORY=$(dirname "$0")
psp-cmake -S "$SCRIPT_DIRECTORY/.." -B "$SCRIPT_DIRECTORY/../psp_build" || exit 1
make --directory="$SCRIPT_DIRECTORY/../psp_build" || exit 1
