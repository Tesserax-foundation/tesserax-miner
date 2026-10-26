# Tesserax miner

This is XMRig 6.26.0 with the `rx/tess` algorithm (RandomTESS), the proof of work of
[Tesserax](https://github.com/Tesserax-labs/tesserax), and **no developer fee**.

- `rx/tess` is RandomX v2 (Monero `rx/2` with the commitment) with the Argon2 salt
  `RandomTESS\x01` and the instruction frequencies IROR_R 6, IROL_R 4, FADD_M 4, FSUB_M 6. CPU
  only: the GPU backends do not implement RandomX v2.
- The donation level is 0% and cannot be raised: no hashrate is ever diverted to another pool.
- Fixes the light mode of RandomX v2 on x86-64: the JIT of XMRig 6.26.0 reads the dataset in light
  mode as in v1 (it swaps "ma" and "mx" before modifying the address), so its light mode hashes
  were wrong for `rx/2` and `rx/tess`. The fast mode was not affected.

Download the archive of your platform from the
[releases](https://github.com/Tesserax-labs/tesserax-miner/releases) and check it against `SHA256SUMS`:

| Platform | Archive |
|---|---|
| Linux x64, arm64 | `tesserax-miner-<version>-linux-<arch>.tar.gz`, static: runs on any distribution |
| macOS Apple Silicon, Intel | `tesserax-miner-<version>-macos-<arch>.tar.gz`, macOS 12 or later |
| Windows x64 | `tesserax-miner-<version>-windows-x64.zip` |

The binaries are not notarized: on macOS, allow the miner once with
`xattr -d com.apple.quarantine xmrig`. Antivirus software often flags miners; the binaries are
built by the `Release` workflow of this repository from its sources, and every one of them is
checked against the reference RandomTESS implementation before it is published.

Or build it:

```bash
mkdir build && cd build
cmake .. -DWITH_OPENCL=OFF -DWITH_CUDA=OFF
make -j$(nproc)
```

Start a Tesserax node with its stratum server, then the miner:

```bash
tesserax --stratum --author <your address>
./xmrig -o 127.0.0.1:3333 -u rig1 -a rx/tess
```

Every block found credits the `--author` of the node. On macOS with Homebrew, add
`-DWITH_HWLOC=OFF -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)` to `cmake`.

`scripts/tesserax/deps.sh` builds the static dependencies of the releases (OpenSSL, libuv,
hwloc), each source archive checked against a pinned SHA-256. `tests/rx-tess/` runs the miner
against a local stratum job and checks its shares with the reference RandomX implementation
built with the RandomTESS configuration of the node: `tests/rx-tess/run.sh` runs it on one's own
machine, in the light and the fast modes. A release is published when a tag
`v<APP_VERSION>` is pushed.

The rest of this file is the XMRig README.

# XMRig

[![Github All Releases](https://img.shields.io/github/downloads/xmrig/xmrig/total.svg)](https://github.com/xmrig/xmrig/releases)
[![GitHub release](https://img.shields.io/github/release/xmrig/xmrig/all.svg)](https://github.com/xmrig/xmrig/releases)
[![GitHub Release Date](https://img.shields.io/github/release-date/xmrig/xmrig.svg)](https://github.com/xmrig/xmrig/releases)
[![GitHub license](https://img.shields.io/github/license/xmrig/xmrig.svg)](https://github.com/xmrig/xmrig/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/xmrig/xmrig.svg)](https://github.com/xmrig/xmrig/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/xmrig/xmrig.svg)](https://github.com/xmrig/xmrig/network)

XMRig is a high performance, open source, cross platform RandomX, KawPow, CryptoNight and [GhostRider](https://github.com/xmrig/xmrig/tree/master/src/crypto/ghostrider#readme) unified CPU/GPU miner and [RandomX benchmark](https://xmrig.com/benchmark). Official binaries are available for Windows, Linux, macOS and FreeBSD.

## Mining backends
- **CPU** (x86/x64/ARMv7/ARMv8/RISC-V)
- **OpenCL** for AMD GPUs.
- **CUDA** for NVIDIA GPUs via external [CUDA plugin](https://github.com/xmrig/xmrig-cuda).

## Download
* **[Binary releases](https://github.com/xmrig/xmrig/releases)**
* **[Build from source](https://xmrig.com/docs/miner/build)**

## Usage
The preferred way to configure the miner is the [JSON config file](https://xmrig.com/docs/miner/config) as it is more flexible and human friendly. The [command line interface](https://xmrig.com/docs/miner/command-line-options) does not cover all features, such as mining profiles for different algorithms. Important options can be changed during runtime without miner restart by editing the config file or executing [API](https://xmrig.com/docs/miner/api) calls.

* **[Wizard](https://xmrig.com/wizard)** helps you create initial configuration for the miner.
* **[Workers](http://workers.xmrig.info)** helps manage your miners via HTTP API.

## Donations
* Upstream XMRig donates 1% of the hashrate by default; this build donates nothing.
* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`

## Developers
* **[xmrig](https://github.com/xmrig)**
* **[sech1](https://github.com/SChernykh)**

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
* [twitter](https://twitter.com/xmrig_dev)
