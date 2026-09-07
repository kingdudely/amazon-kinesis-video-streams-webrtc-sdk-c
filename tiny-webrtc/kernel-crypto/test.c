#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "include/tinycrypto_uapi.h"

static int check_sha256(int fd)
{
    static const uint8_t text[] = "abc";
    static const uint8_t expected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad
    };
    struct tinycrypto_hash_req req = {0};

    req.input = (uintptr_t) text;
    req.input_len = sizeof(text) - 1;
    if (ioctl(fd, TINYCRYPTO_SHA256, &req) != 0)
        return -1;
    return memcmp(req.output, expected, sizeof(expected)) == 0 ? 0 : -1;
}

static int check_random(int fd)
{
    uint8_t data[32] = {0};
    struct tinycrypto_random_req req = {
        .output = (uintptr_t) data,
        .output_len = sizeof(data),
    };
    int all_zero = 1;

    if (ioctl(fd, TINYCRYPTO_RANDOM, &req) != 0)
        return -1;
    for (size_t i = 0; i < sizeof(data); ++i)
        all_zero &= data[i] == 0;
    return all_zero ? -1 : 0;
}

int main(void)
{
    int fd = open("/dev/tinycrypto", O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("open /dev/tinycrypto");
        return 1;
    }

    const int sha_ok = check_sha256(fd) == 0;
    const int rng_ok = check_random(fd) == 0;
    close(fd);

    printf("SHA-256: %s\n", sha_ok ? "ok" : "FAIL");
    printf("random:  %s\n", rng_ok ? "ok" : "FAIL");
    return sha_ok && rng_ok ? 0 : 1;
}
