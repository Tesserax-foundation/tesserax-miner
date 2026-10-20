/*
 * Check rx/tess shares against the reference RandomX implementation built with the RandomTESS
 * configuration (configuration.h, as in the Tesserax node).
 *
 *     verify shares.txt [shares.txt...]
 *
 * Each file starts with `seed <hex>` and has one share per line: `blob result commitment` in hex,
 * where `commitment` is the RandomTESS hash of the blob and `result` its RandomX v2 commitment, as
 * XMRig submits them. Every share must match, and every file must hold at least one share.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "randomx.h"

#define MAX_BLOB 256

static int from_hex(const char *hex, unsigned char *out, size_t max)
{
    size_t len = strlen(hex);
    if (len % 2 != 0 || len / 2 > max) {
        return -1;
    }
    for (size_t i = 0; i < len / 2; i++) {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%2x", &byte) != 1) {
            return -1;
        }
        out[i] = (unsigned char) byte;
    }
    return (int) (len / 2);
}

static void to_hex(const unsigned char *bytes, size_t len, char *out)
{
    for (size_t i = 0; i < len; i++) {
        sprintf(out + 2 * i, "%02x", bytes[i]);
    }
}

static randomx_vm *light_vm(randomx_flags flags, const void *key, size_t key_size, randomx_cache **cache)
{
    *cache = randomx_alloc_cache(flags);
    if (!*cache) {
        fprintf(stderr, "cannot allocate the RandomX cache\n");
        exit(2);
    }
    randomx_init_cache(*cache, key, key_size);
    randomx_vm *vm = randomx_create_vm(flags, *cache, NULL);
    if (!vm) {
        fprintf(stderr, "cannot create the RandomX virtual machine\n");
        exit(2);
    }
    return vm;
}

/* The reference vectors of the Tesserax node (crypto/randomtess/src/tests.rs). */
static void check_configuration(randomx_flags flags)
{
    static const char key[] = "Tesserax RandomTESS test key";
    randomx_cache *cache;
    randomx_vm *vm = light_vm(flags, key, strlen(key), &cache);
    unsigned char hash[RANDOMX_HASH_SIZE], commitment[RANDOMX_HASH_SIZE], input[43];
    char hex[2 * RANDOMX_HASH_SIZE + 1];

    randomx_calculate_hash(vm, "Tesserax", 8, hash);
    to_hex(hash, sizeof(hash), hex);
    if (strcmp(hex, "365e9fdb82a9e7573168c0f432380c3249dbbccf5d4b356d467427bcc9deb277") != 0) {
        fprintf(stderr, "the reference is not RandomTESS: hash %s\n", hex);
        exit(2);
    }

    memset(input, 0x42, sizeof(input));
    randomx_calculate_hash(vm, input, sizeof(input), hash);
    randomx_calculate_commitment(input, sizeof(input), hash, commitment);
    to_hex(commitment, sizeof(commitment), hex);
    if (strcmp(hex, "b5098470799f651832b0244f8487d8c4bba775dd2588bfb76a5e0cda7d5b233a") != 0) {
        fprintf(stderr, "the reference is not RandomTESS: commitment %s\n", hex);
        exit(2);
    }

    randomx_destroy_vm(vm);
    randomx_release_cache(cache);
}

static int verify_file(randomx_flags flags, const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "%s: cannot open\n", path);
        return 1;
    }

    char seed_hex[2 * RANDOMX_HASH_SIZE + 1];
    unsigned char seed[RANDOMX_HASH_SIZE];
    if (fscanf(file, "seed %64s", seed_hex) != 1 || from_hex(seed_hex, seed, sizeof(seed)) != RANDOMX_HASH_SIZE) {
        fprintf(stderr, "%s: missing seed\n", path);
        return 1;
    }

    randomx_cache *cache;
    randomx_vm *vm = light_vm(flags, seed, sizeof(seed), &cache);

    char blob_hex[2 * MAX_BLOB + 1], result_hex[2 * RANDOMX_HASH_SIZE + 1], commitment_hex[2 * RANDOMX_HASH_SIZE + 1];
    int shares = 0, failures = 0;
    while (fscanf(file, "%512s %64s %64s", blob_hex, result_hex, commitment_hex) == 3) {
        unsigned char blob[MAX_BLOB], hash[RANDOMX_HASH_SIZE], commitment[RANDOMX_HASH_SIZE];
        char hash_hex[2 * RANDOMX_HASH_SIZE + 1], value_hex[2 * RANDOMX_HASH_SIZE + 1];
        int size = from_hex(blob_hex, blob, sizeof(blob));
        if (size <= 0) {
            fprintf(stderr, "%s: malformed share %d\n", path, shares);
            failures++;
            break;
        }

        randomx_calculate_hash(vm, blob, (size_t) size, hash);
        randomx_calculate_commitment(blob, (size_t) size, hash, commitment);
        to_hex(hash, sizeof(hash), hash_hex);
        to_hex(commitment, sizeof(commitment), value_hex);

        if (strcmp(hash_hex, commitment_hex) != 0 || strcmp(value_hex, result_hex) != 0) {
            fprintf(stderr, "%s: share %d does not match\n  blob       %s\n  hash       %s (miner %s)\n  commitment %s (miner %s)\n",
                    path, shares, blob_hex, hash_hex, commitment_hex, value_hex, result_hex);
            failures++;
        }
        shares++;
    }
    fclose(file);
    randomx_destroy_vm(vm);
    randomx_release_cache(cache);

    if (shares == 0) {
        fprintf(stderr, "%s: no shares\n", path);
        return 1;
    }
    printf("%s: %d shares, %d mismatches\n", path, shares, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s shares.txt [shares.txt...]\n", argv[0]);
        return 2;
    }

    const randomx_flags flags = randomx_get_flags() | RANDOMX_FLAG_V2;
    check_configuration(flags);

    int failed = 0;
    for (int i = 1; i < argc; i++) {
        failed |= verify_file(flags, argv[i]);
    }
    return failed;
}
