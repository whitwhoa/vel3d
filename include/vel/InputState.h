#pragma once

#include <bitset>

namespace vel
{
    enum VEL_KEY
    {
        VEL_KEY_SPACE = 0,
        VEL_KEY_APOSTROPHE,
        VEL_KEY_COMMA,
        VEL_KEY_MINUS,
        VEL_KEY_PERIOD,
        VEL_KEY_SLASH,

        VEL_KEY_0,
        VEL_KEY_1,
        VEL_KEY_2,
        VEL_KEY_3,
        VEL_KEY_4,
        VEL_KEY_5,
        VEL_KEY_6,
        VEL_KEY_7,
        VEL_KEY_8,
        VEL_KEY_9,

        VEL_KEY_SEMICOLON,
        VEL_KEY_EQUAL,

        VEL_KEY_A,
        VEL_KEY_B,
        VEL_KEY_C,
        VEL_KEY_D,
        VEL_KEY_E,
        VEL_KEY_F,
        VEL_KEY_G,
        VEL_KEY_H,
        VEL_KEY_I,
        VEL_KEY_J,
        VEL_KEY_K,
        VEL_KEY_L,
        VEL_KEY_M,
        VEL_KEY_N,
        VEL_KEY_O,
        VEL_KEY_P,
        VEL_KEY_Q,
        VEL_KEY_R,
        VEL_KEY_S,
        VEL_KEY_T,
        VEL_KEY_U,
        VEL_KEY_V,
        VEL_KEY_W,
        VEL_KEY_X,
        VEL_KEY_Y,
        VEL_KEY_Z,

        VEL_KEY_LEFT_BRACKET,
        VEL_KEY_RIGHT_BRACKET,
        VEL_KEY_BACKSLASH,
        VEL_KEY_GRAVE_ACCENT,

        VEL_KEY_ESCAPE,
        VEL_KEY_ENTER,
        VEL_KEY_TAB,
        VEL_KEY_BACKSPACE,
        VEL_KEY_INSERT,
        VEL_KEY_DELETE,

        VEL_KEY_RIGHT,
        VEL_KEY_LEFT,
        VEL_KEY_DOWN,
        VEL_KEY_UP,

        VEL_KEY_PAGE_UP,
        VEL_KEY_PAGE_DOWN,
        VEL_KEY_HOME,
        VEL_KEY_END,

        VEL_KEY_CAPS_LOCK,
        VEL_KEY_SCROLL_LOCK,
        VEL_KEY_NUM_LOCK,
        VEL_KEY_PRINT_SCREEN,
        VEL_KEY_PAUSE,

        VEL_KEY_F1,
        VEL_KEY_F2,
        VEL_KEY_F3,
        VEL_KEY_F4,
        VEL_KEY_F5,
        VEL_KEY_F6,
        VEL_KEY_F7,
        VEL_KEY_F8,
        VEL_KEY_F9,
        VEL_KEY_F10,
        VEL_KEY_F11,
        VEL_KEY_F12,

        VEL_KEY_KP_0,
        VEL_KEY_KP_1,
        VEL_KEY_KP_2,
        VEL_KEY_KP_3,
        VEL_KEY_KP_4,
        VEL_KEY_KP_5,
        VEL_KEY_KP_6,
        VEL_KEY_KP_7,
        VEL_KEY_KP_8,
        VEL_KEY_KP_9,

        VEL_KEY_KP_DECIMAL,
        VEL_KEY_KP_DIVIDE,
        VEL_KEY_KP_MULTIPLY,
        VEL_KEY_KP_SUBTRACT,
        VEL_KEY_KP_ADD,
        VEL_KEY_KP_ENTER,
        VEL_KEY_KP_EQUAL,

        VEL_KEY_LEFT_SHIFT,
        VEL_KEY_LEFT_CONTROL,
        VEL_KEY_LEFT_ALT,
        VEL_KEY_LEFT_SUPER,

        VEL_KEY_RIGHT_SHIFT,
        VEL_KEY_RIGHT_CONTROL,
        VEL_KEY_RIGHT_ALT,
        VEL_KEY_RIGHT_SUPER,

        VEL_KEY_MENU,

        VEL_KEY_COUNT
    };

    struct InputState
    {
        float mouseSensitivity = 0.0f;
        float mouseDX = 0.0f;
        float mouseDY = 0.0f;
        int   scroll = 0;

        bool mouseLeftButton = false;
        bool mouseRightButton = false;

        std::bitset<VEL_KEY_COUNT> keys;
        std::bitset<VEL_KEY_COUNT> prevKeys;

        void updatePrevKeys()
        {
            this->prevKeys = this->keys;
        }

        void setKey(VEL_KEY key, bool pressed)
        {
            this->keys.set(key, pressed);
        }

        bool down(VEL_KEY key) const
        {
            return this->keys.test(key);
        }

        bool up(VEL_KEY key) const
        {
            return !this->keys.test(key);
        }

        bool pressed(VEL_KEY key) const
        {
            return this->keys.test(key) &&
                !this->prevKeys.test(key);
        }

        bool held(VEL_KEY key) const
        {
            return this->keys.test(key) &&
                this->prevKeys.test(key);
        }

        bool released(VEL_KEY key) const
        {
            return !this->keys.test(key) &&
                this->prevKeys.test(key);
        }

        bool noKeyPressed() const
        {
            return this->keys.none();
        }

    };
}