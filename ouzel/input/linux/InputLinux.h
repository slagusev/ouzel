// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#pragma once

#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/cursorfont.h>
#include <X11/X.h>
#include "input/Input.h"
#include "utils/Types.h"

namespace ouzel
{
    class Engine;

    namespace input
    {
        class InputLinux: public Input
        {
            friend Engine;
        public:
            static KeyboardKey convertKeyCode(KeySym keyCode);
            static uint32_t getModifiers(unsigned int state);

            virtual ~InputLinux();

            virtual void setCursorVisible(bool visible) override;
            virtual bool isCursorVisible() const override;

            virtual void setCursorPosition(const Vector2& position) override;

        protected:
            InputLinux();

            bool cursorVisible = true;
            Cursor emptyCursor = None;
        };
    } // namespace input
} // namespace ouzel
