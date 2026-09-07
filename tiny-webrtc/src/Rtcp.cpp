#include <cstdint>
namespace tinyrtc {
struct RtcpHeader { std::uint8_t version{}; std::uint8_t type{}; std::uint16_t length{}; };
}