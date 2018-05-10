// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cmath>
#include <mutex>
#include "core/frontend/emu_window.h"
#include "core/frontend/input.h"
#include "core/settings.h"

class EmuWindow::TouchState : public Input::Factory<Input::TouchDevice>,
                              public std::enable_shared_from_this<TouchState> {
public:
    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage&) override {
        return std::make_unique<Device>(shared_from_this());
    }

    std::mutex mutex;

    bool touch_pressed = false; ///< True if touchpad area is currently pressed, otherwise false

    float touch_x = 0.0f; ///< Touchpad X-position
    float touch_y = 0.0f; ///< Touchpad Y-position

private:
    class Device : public Input::TouchDevice {
    public:
        explicit Device(std::weak_ptr<TouchState>&& touch_state) : touch_state(touch_state) {}
        std::tuple<float, float, bool> GetStatus() const override {
            if (auto state = touch_state.lock()) {
                std::lock_guard<std::mutex> guard(state->mutex);
                return std::make_tuple(state->touch_x, state->touch_y, state->touch_pressed);
            }
            return std::make_tuple(0.0f, 0.0f, false);
        }

    private:
        std::weak_ptr<TouchState> touch_state;
    };
};

EmuWindow::EmuWindow() {
    touch_state = std::make_shared<TouchState>();
    Input::RegisterFactory<Input::TouchDevice>("emu_window", touch_state);
}

EmuWindow::~EmuWindow() {
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
}

void EmuWindow::TouchPressed(u16 x, u16 y) {
    std::lock_guard<std::mutex> guard(touch_state->mutex);
    touch_state->touch_pressed = true;
    touch_state->touch_x = x;
    touch_state->touch_y = y;
}

void EmuWindow::TouchReleased() {
    std::lock_guard<std::mutex> guard(touch_state->mutex);
    touch_state->touch_pressed = false;
    touch_state->touch_x = 0;
    touch_state->touch_y = 0;
}
