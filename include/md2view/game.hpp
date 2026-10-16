/// @file
/// Game interface driven by an Engine
#pragma once

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
    virtual void processInput(Engine& engine, float deltaTime) = 0;
    virtual void onMouseMovement(float xoffset, float yoffset) = 0;
    virtual void onMouseScroll(double xoffset, double yoffset) = 0;
    virtual void onFramebufferResize() = 0;

    virtual void update(Engine& engine, float deltaTime) = 0;
    virtual void render(Engine& engine) = 0;
};
