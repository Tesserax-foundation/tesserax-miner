#!/bin/sh -e
#
# Build `verify` against the reference RandomX implementation (tevador/RandomX at the commit the
# Tesserax node uses) with the RandomTESS configuration.
#
#     build-verify.sh OUTPUT

RANDOMX_COMMIT="a901d5bff16517ecf5c98c92c233e8480e259fc4"
HERE="$(cd "$(dirname "$0")" && pwd)"
OUTPUT="${1:-$HERE/verify}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

git clone --quiet https://github.com/tevador/RandomX.git "$WORK/RandomX"
git -C "$WORK/RandomX" checkout --quiet "$RANDOMX_COMMIT"
cp "$HERE/configuration.h" "$WORK/RandomX/src/configuration.h"

cmake -S "$WORK/RandomX" -B "$WORK/build" -DCMAKE_BUILD_TYPE=Release -DARCH=default >/dev/null
cmake --build "$WORK/build" --target randomx -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)" >/dev/null

if [ "$(uname)" = "Darwin" ]; then CXXLIB=-lc++; else CXXLIB=-lstdc++; fi
cc -O2 -o "$OUTPUT" "$HERE/verify.c" -I"$WORK/RandomX/src" "$WORK/build/librandomx.a" $CXXLIB -lpthread
echo "built $OUTPUT"
