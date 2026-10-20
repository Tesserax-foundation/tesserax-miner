#!/usr/bin/env python3
"""Run XMRig against a local stratum server serving one fixed rx/tess job, and record its shares.

The job has the layout of a Tesserax node job: a 43-byte blob whose last 4 bytes are the nonce
iterated by the miner, the RandomTESS key as seed hash, and a target that accepts every hash. The
shares are written to OUT, one per line, as `blob result commitment` (hex), after a `seed` line;
`verify` checks them against the reference RandomX implementation.

    stratum.py --xmrig ./xmrig --out shares.txt
"""

import argparse
import hashlib
import json
import os
import socket
import subprocess
import sys
import tempfile
import time

SEED = hashlib.sha256(b"tesserax-miner rx/tess test seed").digest()
# Header pre-hash, then the 7-byte extra nonce of the connection, then the miner nonce.
PRE_HASH = hashlib.sha256(b"tesserax-miner rx/tess test header").digest()
EXTRA_NONCE = bytes.fromhex("01020304050607")
BLOB = PRE_HASH + EXTRA_NONCE + bytes(4)
NONCE_OFFSET = 39


def reply(conn, message):
    conn.sendall((json.dumps(message) + "\n").encode())


def serve(conn, count, deadline):
    """Answer the miner until it submitted `count` shares; return them."""
    shares = []
    buffer = b""
    conn.settimeout(5)
    while len(shares) < count:
        if time.monotonic() > deadline:
            raise TimeoutError(f"only {len(shares)} shares before the timeout")
        try:
            data = conn.recv(65536)
        except socket.timeout:
            continue
        if not data:
            raise ConnectionError("the miner closed the connection")
        buffer += data
        while b"\n" in buffer:
            line, buffer = buffer.split(b"\n", 1)
            request = json.loads(line)
            method, params = request.get("method"), request.get("params") or {}
            if method == "login":
                algorithms = params.get("algo") or []
                if "rx/tess" not in algorithms:
                    raise ValueError(f"the miner does not announce rx/tess: {algorithms}")
                reply(conn, {
                    "id": request["id"], "jsonrpc": "2.0", "error": None,
                    "result": {
                        "id": "test", "status": "OK", "extensions": ["algo", "keepalive"],
                        "job": {
                            "blob": BLOB.hex(), "job_id": "1", "target": "ffffffffffffffff",
                            "algo": "rx/tess", "height": 1, "seed_hash": SEED.hex(),
                        },
                    },
                })
            elif method == "submit":
                nonce = bytes.fromhex(params["nonce"])
                blob = BLOB[:NONCE_OFFSET] + nonce + BLOB[NONCE_OFFSET + 4:]
                shares.append((blob.hex(), params["result"], params.get("commitment", "")))
                reply(conn, {"id": request["id"], "jsonrpc": "2.0", "error": None,
                             "result": {"status": "OK"}})
            elif method == "keepalived":
                reply(conn, {"id": request["id"], "jsonrpc": "2.0", "error": None,
                             "result": {"status": "KEEPALIVED"}})
    return shares


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--xmrig", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--count", type=int, default=32)
    parser.add_argument("--timeout", type=int, default=1200)
    args = parser.parse_args()

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", 0))
    server.listen(1)
    port = server.getsockname()[1]

    log = os.path.join(tempfile.mkdtemp(), "xmrig.log")
    miner = subprocess.Popen(
        [
            args.xmrig, "-o", f"127.0.0.1:{port}", "-u", "test", "-a", "rx/tess",
            "--threads=2", "--randomx-mode=light", "--no-huge-pages", "--randomx-no-numa",
            "--no-color", "--print-time=30", f"--log-file={log}",
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    deadline = time.monotonic() + args.timeout
    try:
        server.settimeout(120)
        conn, _ = server.accept()
        shares = serve(conn, args.count, deadline)
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        if os.path.exists(log):
            print(open(log, errors="replace").read(), file=sys.stderr)
        sys.exit(1)
    finally:
        miner.terminate()
        try:
            miner.wait(timeout=30)
        except subprocess.TimeoutExpired:
            miner.kill()

    with open(args.out, "w") as out:
        out.write(f"seed {SEED.hex()}\n")
        for share in shares:
            out.write(" ".join(share) + "\n")
    print(open(log, errors="replace").read())
    print(f"{len(shares)} shares written to {args.out}")


if __name__ == "__main__":
    main()
