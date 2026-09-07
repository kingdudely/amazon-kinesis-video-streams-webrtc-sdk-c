#pragma once

#include <memory>

#include "CryptoProvider.h"

namespace tinyrtc {

std::unique_ptr<CryptoProvider> createLinuxKernelCrypto(const char* device = "/dev/tinycrypto");

} // namespace tinyrtc
