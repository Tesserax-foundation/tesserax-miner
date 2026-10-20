#!/bin/sh -e
#
# Build the static dependencies of the release binaries into PREFIX (default: scripts/deps):
# OpenSSL, libuv and, unless WITH_HWLOC=0, hwloc. Every source archive is checked against a pinned
# SHA-256 before it is used.

PREFIX="${1:-$(cd "$(dirname "$0")/.." && pwd)/deps}"
WITH_HWLOC="${WITH_HWLOC:-1}"

OPENSSL_VERSION="3.5.9"
OPENSSL_SHA256="603f5602e2eef00d77fbd429d34dcd5822bb301757a1bc9cdb24c670f1eb859a"
UV_VERSION="1.53.0"
UV_SHA256="cb0d6dd2128d5a95bd242c6cc982a24fe608fa93da57b6b4ec763b0018c53e64"
HWLOC_VERSION="2.15.0"
HWLOC_SHA256="f54bbdfc9f8015308c2e7bbab3e7789e47436a4a016689e5749a0c35d6877619"

JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

sha256() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    else
        shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

# fetch URL SHA256: download URL into the work directory and check its digest.
fetch() {
    file="$WORK/$(basename "$1")"
    curl -sSfL --retry 3 -o "$file" "$1"
    actual="$(sha256 "$file")"
    if [ "$actual" != "$2" ]; then
        echo "checksum mismatch for $1: expected $2, got $actual" >&2
        exit 1
    fi
    tar -xzf "$file" -C "$WORK"
}

mkdir -p "$PREFIX"

fetch "https://github.com/openssl/openssl/releases/download/openssl-$OPENSSL_VERSION/openssl-$OPENSSL_VERSION.tar.gz" "$OPENSSL_SHA256"
(
    cd "$WORK/openssl-$OPENSSL_VERSION"
    ./config --prefix="$PREFIX" --libdir=lib no-shared no-asm no-zlib no-comp no-dgram no-filenames no-cms no-tests no-docs
    make -j"$JOBS" build_libs
    make install_dev
)

fetch "https://dist.libuv.org/dist/v$UV_VERSION/libuv-v$UV_VERSION.tar.gz" "$UV_SHA256"
(
    cd "$WORK/libuv-v$UV_VERSION"
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$PREFIX" -DCMAKE_INSTALL_LIBDIR=lib \
        -DLIBUV_BUILD_SHARED=OFF -DLIBUV_BUILD_TESTS=OFF -DLIBUV_BUILD_BENCH=OFF
    cmake --build build -j"$JOBS"
    cmake --install build
)

if [ "$WITH_HWLOC" = "1" ]; then
    fetch "https://download.open-mpi.org/release/hwloc/v${HWLOC_VERSION%.*}/hwloc-$HWLOC_VERSION.tar.gz" "$HWLOC_SHA256"
    (
        cd "$WORK/hwloc-$HWLOC_VERSION"
        ./configure --prefix="$PREFIX" --libdir="$PREFIX/lib" --disable-shared --enable-static \
            --disable-io --disable-libudev --disable-libxml2 --disable-cairo --disable-doxygen
        make -j"$JOBS"
        make install
    )
fi

echo "dependencies installed in $PREFIX"
