/// @file
/// OpenGL engine: window, renderer, GUI, and frame loop
#pragma once

#include "md2view/cli.hpp"
#include "md2view/engine.hpp"
#include "md2view/game.hpp"
#include "md2view/gl/gl.hpp"
#include "md2view/gl/gui.hpp"
#include "md2view/gl/renderer.hpp"

#include <gsl-lite/gsl-lite.hpp>

#include <memory>

namespace GL {

class Engine : public ::Engine {
public:
    /// Creates window, renderer, and GUI, then initializes @p game.
    ///
    /// @throws std::runtime_error if game fails to initialize
    Engine(CliArgs const& args, std::unique_ptr<Game> game);
    ~Engine() override;

    Engine(Engine const&) = delete;
    Engine& operator=(Engine const&) = delete;

    Engine(Engine&&) noexcept = delete;
    Engine& operator=(Engine&&) noexcept = delete;

    void runGame();

    [[nodiscard]] Renderer& renderer() override {
        gsl_Expects(renderer_);
        return *renderer_;
    }

private:
    void onKey(int key, int scancode, int action, int mods) final;
    void onCursorPos(double xpos, double ypos) final;
    void onScroll(double xoffset, double yoffset) final;
    void onMouseButton(int button, int action, int mods) final;
    void onChar(unsigned int codepoint) final;
    [[nodiscard]] bool guiHasInput() const { return gui_ && !inputGoesToGame_; }

    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<GL::Gui> gui_;
    std::unique_ptr<Game> game_;
};

} // namespace GL
