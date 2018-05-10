// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <tuple>
#include <utility>
#include <vector>
#include "common/common_types.h"
#include "core/frontend/framebuffer.h"

/**
 * Abstraction class used to provide an interface between emulation code and the frontend
 * (e.g. SDL, QGLWidget, GLFW, etc...).
 *
 * Design notes on the interaction between EmuWindow and the emulation core:
 * - Generally, decisions on anything visible to the user should be left up to the GUI.
 *   For example, the emulation core should not try to dictate some window title or size.
 *   This stuff is not the core's business and only causes problems with regards to thread-safety
 *   anyway.
 * - Under certain circumstances, it may be desirable for the core to politely request the GUI
 *   to set e.g. a minimum window size. However, the GUI should always be free to ignore any
 *   such hints.
 * - EmuWindow may expose some of its state as read-only to the emulation core, however care
 *   should be taken to make sure the provided information is self-consistent. This requires
 *   some sort of synchronization (most of this is still a TODO).
 * - DO NOT TREAT THIS CLASS AS A GUI TOOLKIT ABSTRACTION LAYER. That's not what it is. Please
 *   re-read the upper points again and think about it if you don't see this.
 */
class EmuWindow {
public:
    /// Swap buffers to display the next frame
    virtual void SwapBuffers() = 0;

    /// Polls window events
    virtual void PollEvents() = 0;

    /**
     * Called by a Framebuffer to set the touch state for the core.
     * @param x Native 3ds x coordinate
     * @param y Native 3ds y coordinate
     */
    void TouchPressed(u16 x, u16 y);

    /**
     * Called by a Framebuffer to clear any touch state
     */
    void TouchReleased();

    std::vector<std::shared_ptr<Framebuffer>>& GetFramebuffer() {
        return screens;
    }

    const std::vector<std::shared_ptr<Framebuffer>>& GetFramebuffer() const {
        return screens;
    }

protected:
    EmuWindow();
    virtual ~EmuWindow();

    std::vector<std::shared_ptr<Framebuffer>> screens;

private:
    class TouchState;
    std::shared_ptr<TouchState> touch_state;
};
