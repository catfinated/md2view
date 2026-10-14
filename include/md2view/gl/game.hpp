#pragma once

#include "md2view/gl/gl.hpp"

namespace GL {

class Engine;

/// Game interface
class Game {
public:
    Game() = default;
    virtual ~Game() = default;

    Game(Game const&) = delete;
    Game& operator=(Game const&) = delete;

    Game(Game&&) noexcept = delete;
    Game& operator=(Game&&) noexcept = delete;

    /// Window title. Called before the window is created.
    [[nodiscard]] virtual char const* title() const = 0;

    /// Called once the renderer and GUI exist. Return false to abort.
    [[nodiscard]] virtual bool onEngineInitialized(Engine& engine) = 0;

    /// Called each frame while input goes to the game
    virtual void processInput(Engine& engine, GLfloat deltaTime) = 0;
    virtual void onMouseMovement(GLfloat xoffset, GLfloat yoffset) = 0;
    virtual void onMouseScroll(double xoffset, double yoffset) = 0;
    virtual void onFramebufferResize() = 0;

    virtual void update(Engine& engine, GLfloat deltaTime) = 0;
    virtual void render(Engine& engine) = 0;
};

} // namespace GL
