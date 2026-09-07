#pragma once

#include <linux/ioctl.h>
#include <linux/types.h>

#define TINYCRYPTO_IOC_MAGIC 0xC7

#define TINYCRYPTO_SHA1_SIZE    20
#define TINYCRYPTO_SHA256_SIZE  32
#define TINYCRYPTO_GCM_TAG_SIZE 16
#define TINYCRYPTO_GCM_IV_SIZE  12
#define TINYCRYPTO_MAX_KEY_SIZE 32

struct tinycrypto_hash_req {
    __u64 input;
    __u32 input_len;
    __u8 output[TINYCRYPTO_SHA256_SIZE];
};

struct tinycrypto_hmac_req {
    __u64 key;
    __u32 key_len;
    __u64 input;
    __u32 input_len;
    __u8 output[TINYCRYPTO_SHA256_SIZE];
};

struct tinycrypto_gcm_req {
    __u64 data;
    __u32 data_len;
    __u64 aad;
    __u32 aad_len;
    __u8 key[TINYCRYPTO_MAX_KEY_SIZE];
    __u8 key_len;
    __u8 iv[TINYCRYPTO_GCM_IV_SIZE];
    __u8 tag[TINYCRYPTO_GCM_TAG_SIZE];
};

struct tinycrypto_random_req {
    __u64 output;
    __u32 output_len;
};

#define TINYCRYPTO_SHA1        _IOWR(TINYCRYPTO_IOC_MAGIC, 1, struct tinycrypto_hash_req)
#define TINYCRYPTO_SHA256      _IOWR(TINYCRYPTO_IOC_MAGIC, 2, struct tinycrypto_hash_req)
#define TINYCRYPTO_HMAC_SHA1   _IOWR(TINYCRYPTO_IOC_MAGIC, 3, struct tinycrypto_hmac_req)
#define TINYCRYPTO_HMAC_SHA256 _IOWR(TINYCRYPTO_IOC_MAGIC, 4, struct tinycrypto_hmac_req)
#define TINYCRYPTO_AES_GCM_ENC _IOWR(TINYCRYPTO_IOC_MAGIC, 5, struct tinycrypto_gcm_req)
#define TINYCRYPTO_AES_GCM_DEC _IOWR(TINYCRYPTO_IOC_MAGIC, 6, struct tinycrypto_gcm_req)
#define TINYCRYPTO_RANDOM      _IOW(TINYCRYPTO_IOC_MAGIC, 7, struct tinycrypto_random_req)
