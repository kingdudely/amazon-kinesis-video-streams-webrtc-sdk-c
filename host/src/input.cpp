#include "input.hpp"

#include <iostream>

#ifdef _WIN32
#include <windows.h>

namespace {

void mouseButton(uint8_t button, bool down) {
    DWORD flag = 0;
    switch (button) {
    case 0: flag = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP; break;
    case 1: flag = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP; break;
    case 2: flag = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP; break;
    default: return;
    }

    INPUT input{};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flag;
    SendInput(1, &input, sizeof(input));
}

}
#endif

void RemoteInput::pointerMove(bool relative, int32_t x, int32_t y) {
#ifdef _WIN32
    INPUT input{};
    input.type = INPUT_MOUSE;
    if (relative) {
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
    } else {
        input.mi.dx = x;
        input.mi.dy = y;
        input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
        input.mi.dx = static_cast<LONG>((static_cast<int64_t>(x) * 65535) / GetSystemMetrics(SM_CXVIRTUALSCREEN));
        input.mi.dy = static_cast<LONG>((static_cast<int64_t>(y) * 65535) / GetSystemMetrics(SM_CYVIRTUALSCREEN));
    }
    SendInput(1, &input, sizeof(input));
#else
    (void)relative; (void)x; (void)y;
#endif
}

void RemoteInput::pointerButton(bool down, uint8_t button) {
#ifdef _WIN32
    mouseButton(button, down);
#else
    (void)down; (void)button;
#endif
}

void RemoteInput::scroll(uint8_t mode, float x, float y, float z) {
#ifdef _WIN32
    INPUT input{};
    input.type = INPUT_MOUSE;
    if (mode == 0) {
        if (y != 0) {
            input.mi.mouseData = static_cast<DWORD>(static_cast<int>(y));
            input.mi.dwFlags = MOUSEEVENTF_WHEEL;
            SendInput(1, &input, sizeof(input));
        }
        if (x != 0) {
            input.mi.mouseData = static_cast<DWORD>(static_cast<int>(x));
            input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
            SendInput(1, &input, sizeof(input));
        }
    }
#else
    (void)mode; (void)x; (void)y; (void)z;
#endif
}

void RemoteInput::key(uint8_t code, bool down) {
#ifdef _WIN32
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = code;
    input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
    SendInput(1, &input, sizeof(input));
#else
    (void)code; (void)down;
#endif
}

void RemoteInput::setClipboard(const std::string& text) {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (memory) {
        auto* data = static_cast<char*>(GlobalLock(memory));
        if (data) {
            std::copy(text.begin(), text.end(), data);
            data[text.size()] = '\0';
            GlobalUnlock(memory);
            SetClipboardData(CF_TEXT, memory);
            memory = nullptr;
        }
    }
    if (memory) GlobalFree(memory);
    CloseClipboard();
#else
    (void)text;
#endif
}

std::string RemoteInput::getClipboard() const {
#ifdef _WIN32
    if (!OpenClipboard(nullptr)) return {};
    std::string result;
    HANDLE handle = GetClipboardData(CF_TEXT);
    if (handle) {
        const char* text = static_cast<const char*>(GlobalLock(handle));
        if (text) {
            result = text;
            GlobalUnlock(handle);
        }
    }
    CloseClipboard();
    return result;
#else
    return {};
#endif
}
