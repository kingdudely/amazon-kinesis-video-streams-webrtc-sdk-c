#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace tinyrtc::platform {

class WebSocketServer {
public:
    using OpenHandler = std::function<void()>;
    using BinaryHandler = std::function<void(std::span<const std::uint8_t>)>;
    using CloseHandler = std::function<void()>;

    bool listen(std::uint16_t port, const char* token);
    void poll(int timeoutMs);
    void close();
    bool connected() const noexcept;

    void setHandlers(OpenHandler, BinaryHandler, CloseHandler);

private:
    void* impl_{};
};

}