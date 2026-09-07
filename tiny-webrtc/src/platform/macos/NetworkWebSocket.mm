#include "rtc/platform/WebSocket.h"
namespace tinyrtc::platform {
bool WebSocketServer::listen(std::uint16_t, const char*) { return true; }
void WebSocketServer::poll(int) {}
void WebSocketServer::close() {}
bool WebSocketServer::connected() const noexcept { return false; }
void WebSocketServer::setHandlers(OpenHandler, BinaryHandler, CloseHandler) {}
}