#pragma once

#include "md2view/engine.hpp"
#include "md2view/gl/gl.hpp"
#include "md2view/gl/gui.hpp"
#include "md2view/gl/renderer.hpp"
#include "md2view/gl/resource_manager.hpp"

#include <gsl-lite/gsl-lite.hpp>

#include <memory>

namespace GL {

template <typename Game> class Engine : public ::Engine {
public:
    Engine() = default;

    void runGame();

    ResourceManager& resourceManager() {
        gsl_Expects(resourceManager_);
        return *resourceManager_;
    }

    Renderer& renderer() {
        gsl_Expects(renderer_);
        return *renderer_;
    }

private:
    void doInit() final;
    void onKey(int key, int scancode, int action, int mods) final;
    void onCursorPos(double xpos, double ypos) final;
    void onScroll(double xoffset, double yoffset) final;
    void onMouseButton(int button, int action, int mods) final;
    void onChar(unsigned int codepoint) final;
    [[nodiscard]] bool guiHasInput() const { return gui_ && !inputGoesToGame_; }
    [[nodiscard]] GLfloat deltaTime() const { return deltaTime_; }

    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<ResourceManager> resourceManager_;
    std::unique_ptr<GL::Gui> gui_;
    Game game_;
    GLfloat deltaTime_ = 0.0f;
    GLfloat lastFrame_ = 0.0f;
};

} // namespace GL
