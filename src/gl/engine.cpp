#include "md2view/gl/engine.hpp"

#include "md2view/engine.hpp"
#include "md2view/gl/game.hpp"
#include "md2view/gl/gl.hpp"
#include "md2view/gl/gui.hpp"
#include "md2view/gl/renderer.hpp"
#include "md2view/gl/resource_manager.hpp"
#include "md2view/window.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

GL::Engine::Engine(std::unique_ptr<Game> game)
    : game_{std::move(game)} {
    gsl_Expects(game_);
}

GL::Engine::~Engine() = default;

void GL::Engine::doInit() {
    std::optional<std::filesystem::path> pak;
    if (!pakPath_.empty()) {
        pak = pakPath_;
    }

    resourceManager_ = std::make_unique<ResourceManager>("data", pak);
    window_ =
        Window::create(width_, height_, game_->title(), Renderer::kWindowHints);
    renderer_ = std::make_unique<Renderer>(window_, *resourceManager_);

    window_.setFramebufferResizeHandler([this](int width, int height) {
        spdlog::debug("framebuffer resize x={} y={}", width, height);
        if (width > 0 && height > 0) {
            game_->onFramebufferResize();
            renderer_->onFramebufferResize(
                Window::Extent{.width = width, .height = height});
        }
    });

    auto const wSize = window_.size();
    mouse_.xpos = wSize.width / 2.0;
    mouse_.ypos = wSize.height / 2.0;

    gui_ = std::make_unique<GL::Gui>(gsl_lite::not_null{window_.get()});
    glCheckError();
    if (!game_->onEngineInitialized(*this)) {
        throw std::runtime_error("failed to initialize game");
    }
    glCheckError();
}

void GL::Engine::runGame() {
    lastFrame_ = gsl_lite::narrow_cast<GLfloat>(glfwGetTime());

    while (!window_.shouldClose()) {
        auto const currentFrame = gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
        deltaTime_ = currentFrame - lastFrame_;
        lastFrame_ = currentFrame;

        beginFrame();
        glfwPollEvents();

        if (inputGoesToGame_) {
            game_->processInput(*this, deltaTime_);
        }

        gui_->update();
        glCheckError();
        game_->update(*this, deltaTime_);
        glCheckError();
        renderer_->beginFrame();
        game_->render(*this);
        glCheckError();
        renderer_->postProcessFrame();
        gui_->render();
        glCheckError();
        renderer_->endFrame();
    }

    glCheckError();
}

void GL::Engine::onKey(int key, int scancode, int action, int mods) {
    if (guiHasInput()) {
        gui_->onKey(key, scancode, action, mods);
        if (key == GLFW_KEY_ESCAPE && gui_->wantsTextInput()) {
            return;
        }
    }

    auto const gameHadInput = inputGoesToGame_;
    ::Engine::onKey(key, scancode, action, mods);
    if (gui_ && inputGoesToGame_ != gameHadInput) {
        gui_->onInputFocus(!inputGoesToGame_);
    }
}

void GL::Engine::onCursorPos(double xpos, double ypos) {
    ::Engine::onCursorPos(xpos, ypos);
    if (guiHasInput()) {
        gui_->onCursorPos(xpos, ypos);
    } else if (inputGoesToGame_) {
        game_->onMouseMovement(gsl_lite::narrow_cast<float>(mouse_.xoffset),
                               gsl_lite::narrow_cast<float>(mouse_.yoffset));
    }
}

void GL::Engine::onScroll(double xoffset, double yoffset) {
    ::Engine::onScroll(xoffset, yoffset);
    if (guiHasInput()) {
        gui_->onScroll(xoffset, yoffset);
    } else if (inputGoesToGame_) {
        game_->onMouseScroll(xoffset, yoffset);
    }
}

void GL::Engine::onMouseButton(int button, int action, int mods) {
    if (guiHasInput()) {
        gui_->onMouseButton(button, action, mods);
    }
}

void GL::Engine::onChar(unsigned int codepoint) {
    if (guiHasInput()) {
        gui_->onChar(codepoint);
    }
}
