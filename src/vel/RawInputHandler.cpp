#include "vel/RawInputHandler.h"
#include <iostream>
#include <vector>
#include <cstdio>

RawInputHandler::RawInputHandler(HWND hwnd) : mouseX(0), mouseY(0), scrollDelta(0), leftMouseButton(false), rightMouseButton(false) {
    RegisterDevices(hwnd);
}

void RawInputHandler::Update() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_INPUT) {
            ProcessRawInput(msg.lParam);
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

RawInputHandler::InputState RawInputHandler::GetState() const {
    return { mouseX, mouseY, scrollDelta, leftMouseButton, rightMouseButton, keyStates };
}

void RawInputHandler::RegisterDevices(HWND hwnd) {
    //RAWINPUTDEVICE rid[2];
    //// Mouse
    //rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    //rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    //rid[0].dwFlags = RIDEV_INPUTSINK;
    //rid[0].hwndTarget = nullptr;  // Set to nullptr to receive input even when the window is not in the foreground

    //// Keyboard
    //rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    //rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    //rid[1].dwFlags = RIDEV_INPUTSINK;
    //rid[1].hwndTarget = nullptr;  // Set to nullptr to receive input even when the window is not in the foreground

    //// Mouse
    //rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    //rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    //rid[0].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    //rid[0].hwndTarget = GetConsoleWindow();

    //// Keyboard
    //rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    //rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    //rid[1].dwFlags = RIDEV_INPUTSINK | RIDEV_DEVNOTIFY;
    //rid[1].hwndTarget = GetConsoleWindow();

    //std::cout << "Registering raw input devices..." << std::endl;
    //std::cout << "Mouse: usUsagePage=" << rid[0].usUsagePage << ", usUsage=" << rid[0].usUsage
    //    << ", dwFlags=" << rid[0].dwFlags << ", hwndTarget=" << rid[0].hwndTarget << std::endl;
    //std::cout << "Keyboard: usUsagePage=" << rid[1].usUsagePage << ", usUsage=" << rid[1].usUsage
    //    << ", dwFlags=" << rid[1].dwFlags << ", hwndTarget=" << rid[1].hwndTarget << std::endl;

    //RAWINPUTDEVICE rid;

    //// Mouse
    //rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
    //rid.usUsage = HID_USAGE_GENERIC_MOUSE;
    //rid.dwFlags = RIDEV_INPUTSINK;
    //rid.hwndTarget = GetConsoleWindow();  // Use GetConsoleWindow for hwndTarget

    //if (rid.hwndTarget == NULL) {
    //    std::cerr << "GetConsoleWindow() returned NULL" << std::endl;
    //}

    //std::cout << "Registering raw input device..." << std::endl;
    //std::cout << "Mouse: usUsagePage=" << rid.usUsagePage << ", usUsage=" << rid.usUsage
    //    << ", dwFlags=" << rid.dwFlags << ", hwndTarget=" << rid.hwndTarget << std::endl;


    RAWINPUTDEVICE rid[2];
    // Mouse
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwnd;

    // Keyboard
    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[1].dwFlags = RIDEV_INPUTSINK;
    rid[1].hwndTarget = hwnd;

    std::cout << "Registering raw input devices..." << std::endl;
    std::cout << "Mouse: usUsagePage=" << rid[0].usUsagePage << ", usUsage=" << rid[0].usUsage
        << ", dwFlags=" << rid[0].dwFlags << ", hwndTarget=" << rid[0].hwndTarget << std::endl;
    std::cout << "Keyboard: usUsagePage=" << rid[1].usUsagePage << ", usUsage=" << rid[1].usUsage
        << ", dwFlags=" << rid[1].dwFlags << ", hwndTarget=" << rid[1].hwndTarget << std::endl;





    //if (!RegisterRawInputDevices(rid, 2, sizeof(rid[0]))) 
    //if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
    if (RegisterRawInputDevices(rid, 2, sizeof(rid[0])) == FALSE)
    {
        DWORD error = GetLastError();
        //std::cout << "Failed to register raw input devices\n";
        std::cerr << "Failed to register raw input devices. Error: " << error << std::endl;
        std::cin.get();
        exit(EXIT_FAILURE);

    }

    std::cout << "Raw input devices registered successfully." << std::endl;

}

void RawInputHandler::ProcessRawInput(LPARAM lParam) {
    UINT dwSize;
    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    std::vector<BYTE> lpb(dwSize);
    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
        return;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb.data();

    if (raw->header.dwType == RIM_TYPEMOUSE) {
        ProcessMouseInput(raw->data.mouse);
    }
    else if (raw->header.dwType == RIM_TYPEKEYBOARD) {
        ProcessKeyboardInput(raw->data.keyboard);
    }
}

void RawInputHandler::ProcessMouseInput(const RAWMOUSE& rawMouse) {
    if (rawMouse.usFlags == MOUSE_MOVE_ABSOLUTE) {
        mouseX = rawMouse.lLastX;
        mouseY = rawMouse.lLastY;
    }
    else {
        mouseX += rawMouse.lLastX;
        mouseY += rawMouse.lLastY;
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_WHEEL) {
        scrollDelta += static_cast<short>(rawMouse.usButtonData);
    }

    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
        leftMouseButton = true;
    }
    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) {
        leftMouseButton = false;
    }
    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
        rightMouseButton = true;
    }
    if (rawMouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) {
        rightMouseButton = false;
    }
}

void RawInputHandler::ProcessKeyboardInput(const RAWKEYBOARD& rawKeyboard) {
    bool isKeyDown = !(rawKeyboard.Flags & RI_KEY_BREAK);
    USHORT key = rawKeyboard.VKey;

    if (key == 255) return;  // Ignore fake keys

    if (keyStates.find(key) == keyStates.end() || keyStates[key] != isKeyDown) {
        keyStates[key] = isKeyDown;
    }
}