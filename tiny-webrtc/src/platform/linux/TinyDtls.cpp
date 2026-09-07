#include "rtc/dtls/Dtls.h"

#if defined(TINYRTC_WINDOWS)
// Schannel implementation will live here.
#elif defined(TINYRTC_MACOS)
// Network.framework DTLS implementation will live here.
#elif defined(TINYRTC_LINUX)
// Minimal Linux DTLS implementation will live here.
#endif
