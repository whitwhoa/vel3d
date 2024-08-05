#pragma once

#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX            // Prevent windows.h from defining min and max macros
#include <Windows.h>
#include <unordered_map>

// Define HID usage page and usage ID constants if they are not defined
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC ((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE ((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD ((USHORT) 0x06)
#endif

class RawInputHandler {
public:
    struct InputState {
        int mouseX;
        int mouseY;
        int scrollDelta;
        bool leftMouseButton;
        bool rightMouseButton;
        std::unordered_map<USHORT, bool> keyStates;
    };

    RawInputHandler(HWND hwnd);
    void Update();
    InputState GetState() const;

private:
    int mouseX, mouseY, scrollDelta;
    bool leftMouseButton, rightMouseButton;
    std::unordered_map<USHORT, bool> keyStates;

    void RegisterDevices(HWND hwnd);
    void ProcessRawInput(LPARAM lParam);
    void ProcessMouseInput(const RAWMOUSE& rawMouse);
    void ProcessKeyboardInput(const RAWKEYBOARD& rawKeyboard);
};