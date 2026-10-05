/**
 * @brief Types and functions related to windowing
 *
 * Shared by the OpenGL and Vulkan backends. GLFW is deliberately not included
 * here since glfw3.h pulls in GL/gl.h, which must not precede GL/glew.h.
 */
#pragma once

#include <functional>
#include <span>

struct GLFWwindow;

/**
 * @brief RAII guard for glfwInit/glfwTerminate
 *
 * Declare before any object that uses GLFW so that it is destroyed last.
 */
struct GlfwContext {
    /// @throws std::runtime_error if GLFW could not be initialized
    GlfwContext();
    ~GlfwContext();
    GlfwContext(GlfwContext const&) = delete;
    GlfwContext& operator=(GlfwContext const&) = delete;
};

/**
 * @brief A GLFW window creation hint, see glfwWindowHint
 */
struct WindowHint {
    int hint;
    int value;
};

/**
 * @brief Receives input events from a Window
 *
 * Arguments are passed through unchanged from the corresponding GLFW
 * callbacks. Every handler defaults to a no-op so listeners only override
 * the events they care about.
 */
class InputListener {
public:
    InputListener() = default;
    InputListener(InputListener const&) = default;
    InputListener& operator=(InputListener const&) = default;
    InputListener(InputListener&&) = default;
    InputListener& operator=(InputListener&&) = default;
    virtual ~InputListener() = default;

    virtual void
    onKey(int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/) {}
    virtual void onMouseButton(int /*button*/, int /*action*/, int /*mods*/) {}
    virtual void onCursorPos(double /*xpos*/, double /*ypos*/) {}
    virtual void onScroll(double /*xoffset*/, double /*yoffset*/) {}
};

/**
 * @brief RAII wrapper for a GLFW window
 *
 * For an OpenGL window this also owns the GL context, so it must outlive
 * every object that makes GL calls in its destructor.
 *
 * The Window is the only owner of the GLFW user pointer and callbacks for its
 * window. Input events are forwarded to an InputListener and resize events to
 * the registered handlers, so input and rendering concerns can be handled by
 * different objects.
 */
class Window {
public:
    using ResizeHandler = std::function<void(int width, int height)>;

    explicit Window(GLFWwindow* window = nullptr) noexcept;
    ~Window() noexcept;

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    // NB: moves re-point the GLFW user pointer at the new object
    Window(Window&& rhs) noexcept;
    Window& operator=(Window&& rhs) noexcept;

    [[nodiscard]] bool shouldClose() const noexcept;
    void requestClose() noexcept;

    /// Set the receiver of input events, or nullptr for none. The listener
    /// must outlive the Window or be cleared before it is destroyed.
    void setInputListener(InputListener* listener) noexcept;

    /// Called with the new framebuffer size in pixels
    void setFramebufferResizeHandler(ResizeHandler handler);

    /// Called with the new window size in screen coordinates
    void setWindowResizeHandler(ResizeHandler handler);

    [[nodiscard]] GLFWwindow* get() const noexcept { return window_; }

    /// Create a window. Hints are applied on top of the GLFW defaults.
    ///
    /// @throws std::runtime_error if the window could not be created
    static Window create(int width,
                         int height,
                         char const* title,
                         std::span<WindowHint const> hints = {});

private:
    void installCallbacks() noexcept;

    GLFWwindow* window_;
    InputListener* input_{nullptr};
    ResizeHandler onFramebufferResize_;
    ResizeHandler onWindowResize_;
};
