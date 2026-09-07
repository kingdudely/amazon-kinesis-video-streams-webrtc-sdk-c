# Tiny Linux kernel crypto provider

This module exposes only the crypto operations needed by the tiny WebRTC stack through `/dev/tinycrypto`.

The implementation uses the Linux Kernel Crypto API directly from kernel space.

Supported operations:

- SHA-1
- SHA-256
- HMAC-SHA1
- HMAC-SHA256
- AES-GCM with 128/192/256-bit keys and a 96-bit IV
- kernel random bytes

Build:

    make

Load on a development machine:

    sudo insmod tinycrypto.ko

The module creates `/dev/tinycrypto` with mode 0600.

Unload:

    sudo rmmod tinycrypto

The userspace WebRTC library uses `createLinuxKernelCrypto()` to open the device and issue the small fixed ioctl interface.

This module is intentionally narrow. It is not a general crypto service and it is not used through AF_ALG.
