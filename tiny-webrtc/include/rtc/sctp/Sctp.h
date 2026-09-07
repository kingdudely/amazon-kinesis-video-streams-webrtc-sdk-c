#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

namespace tinyrtc {

class Sctp {
public:
    using MessageHandler = std::function<void(std::uint16_t, const std::uint8_t*, std::size_t)>;
    bool init();
    bool input(const std::uint8_t* data, std::size_t size);
    bool send(std::uint16_t streamId, const std::uint8_t* data, std::size_t size);
    void setMessageHandler(MessageHandler handler);

private:
    MessageHandler handler_;
};

}