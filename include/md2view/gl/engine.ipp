#include "md2view/gl/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <array>
#include <utility>

template <typename Game> void GL::Engine<Game>::doInit() {
    std::optional<std::filesystem::path> pak;
    if (!pak_path_.empty()) {
        pak = pak_path_;
    }

    resource_manager_ = std::make_unique<ResourceManager>("data", pak);
    window_ =
        Window::create(width_, height_, game_.title(), Renderer::kWindowHints);
    renderer_ = std::make_unique<Renderer>(window_, *resource_manager_);

    glfwSetInputMode(window_.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    window_.setFramebufferResizeHandler([this](int width, int height) {
        spdlog::debug("framebuffer resize x={} y={}", width, height);
        if (width > 0 && height > 0) {
            game_.onFramebufferResize();
            renderer_->onFramebufferResize(Window::Extent{width, height});
        }
    });

    auto const wSize = window_.size();
    mouse_.xpos = wSize.width / 2.0;
    mouse_.ypos = wSize.height / 2.0;

    gui_ = std::make_unique<GL::Gui>(*this, *resource_manager_,
                                     gsl_lite::not_null{window_.get()});
    glCheckError();
    if (!game_.on_engine_initialized(*this)) {
        throw std::runtime_error("failed to initialize game");
    }
    glCheckError();
}

template <typename Game> void GL::Engine<Game>::run_game() {
    last_frame_ = gsl_lite::narrow_cast<GLfloat>(glfwGetTime());

    while (!window_.shouldClose()) {
        auto const current_frame =
            gsl_lite::narrow_cast<GLfloat>(glfwGetTime());
        delta_time_ = current_frame - last_frame_;
        last_frame_ = current_frame;

        beginFrame();
        glfwPollEvents();

        if (input_goes_to_game_) {
            game_.process_input(*this, delta_time_);
        }

        gui_->update(current_frame, !input_goes_to_game_);
        glCheckError();
        game_.update(*this, delta_time_);
        glCheckError();
        renderer_->beginFrame();
        game_.render(*this);
        glCheckError();
        renderer_->postProcessFrame();
        gui_->render();
        glCheckError();
        renderer_->endFrame();
    }

    glCheckError();
}

template <typename Game>
void GL::Engine<Game>::onCursorPos(double xpos, double ypos) {
    ::Engine::onCursorPos(xpos, ypos);
    if (input_goes_to_game_) {
        game_.on_mouse_movement(gsl_lite::narrow_cast<float>(mouse_.xoffset),
                                gsl_lite::narrow_cast<float>(mouse_.yoffset));
    }
}

template <typename Game>
void GL::Engine<Game>::onScroll(double xoffset, double yoffset) {
    ::Engine::onScroll(xoffset, yoffset);
    if (input_goes_to_game_) {
        game_.on_mouse_scroll(xoffset, yoffset);
    }
}

template <typename Game>
void GL::Engine<Game>::onMouseButton(int button, int action, int /*mods*/) {
    if (gui_) {
        gui_->onMouseButton(button, action);
    }
}
