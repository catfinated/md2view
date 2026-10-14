#include "md2view/engine.hpp"
#include "md2view/cli.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <utility>

bool Engine::Keyboard::keyWasJustPressed(unsigned int key) const {
    gsl_Expects(key < kMaxKeys);
    return justPressed_[key];
}

void Engine::Keyboard::onKey(int key, int action) {
    if (key < 0 || !std::cmp_less(key, kMaxKeys)) {
        return;
    }

    if (action == GLFW_PRESS) {
        keys_[key] = true;
        justPressed_[key] = true;
    } else if (action == GLFW_RELEASE) {
        keys_[key] = false;
    }
}

Engine::Engine(CliArgs const& args)
    : width_{args.width}
    , height_{args.height}
    , assets_{"data", args.pak} {}

void Engine::beginFrame() {
    keyboard_.beginFrame();
    mouse_.xoffset = 0.0;
    mouse_.yoffset = 0.0;
    mouse_.scrollXOffset = 0.0;
    mouse_.scrollYOffset = 0.0;
}

void Engine::onKey(int key, int /*scancode*/, int action, int /*mods*/) {
    keyboard_.onKey(key, action);

    // NB: react to this event rather than polling keyWasJustPressed, which
    // stays true for the rest of the frame and would fire again on every
    // other key event
    if (action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        window_.requestClose();
    } else if (key == GLFW_KEY_F1) {
        inputGoesToGame_ = !inputGoesToGame_;
        spdlog::info("got F1. game input: {}", inputGoesToGame_);
        if (inputGoesToGame_) {
            // NB: ImGui sets a custom cursor and GLFW 3.4 on
            // Wayland keeps animating even when the cursor is
            // disabled so we need to clear it here. ImGui
            // will reset the cursor on the first from after
            // after it is set back to normal
            glfwSetCursor(window_.get(), nullptr);
        }
        glfwSetInputMode(window_.get(), GLFW_CURSOR,
                         inputGoesToGame_ ? GLFW_CURSOR_DISABLED
                                          : GLFW_CURSOR_NORMAL);
    }
}

void Engine::onCursorPos(double xpos, double ypos) {
    mouse_.xoffset = xpos - mouse_.xpos.value_or(xpos);
    // reversed since y-coords go from bottom to top
    mouse_.yoffset = mouse_.ypos.value_or(ypos) - ypos;
    mouse_.xpos = xpos;
    mouse_.ypos = ypos;
}

void Engine::onScroll(double xoffset, double yoffset) {
    mouse_.scrollXOffset += xoffset;
    mouse_.scrollYOffset += yoffset;
}
