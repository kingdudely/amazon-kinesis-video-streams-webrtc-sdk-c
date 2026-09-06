#pragma once

#include <cstdint>
#include <string>

class RemoteInput {
public:
    void pointerMove(bool relative, int32_t x, int32_t y);
    void pointerButton(bool down, uint8_t button);
    void scroll(uint8_t mode, float x, float y, float z);
    void key(uint8_t code, bool down);
    void setClipboard(const std::string& text);
    std::string getClipboard() const;
};
