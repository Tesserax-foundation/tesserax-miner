#!/bin/sh -e
#
# The test of the releases, on one's own machine: the miner mines a fixed rx/tess job in the light
# and the fast modes of RandomX, and every share is checked against the reference RandomX
# implementation built with the RandomTESS configuration of the node.
#
#     tests/rx-tess/run.sh [XMRIG]        # build/xmrig by default

HERE="$(cd "$(dirname "$0")" && pwd)"
XMRIG="${1:-$HERE/../../build/xmrig}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

"$HERE/build-verify.sh" "$WORK/verify"
for mode in light fast; do
	python3 "$HERE/stratum.py" --xmrig "$XMRIG" --mode "$mode" --out "$WORK/shares-$mode.txt"
done
"$WORK/verify" "$WORK"/shares-*.txt
