#pragma once

#include "md2view/asset_loader.hpp"
#include "md2view/cli.hpp"
#include "md2view/window.hpp"

#include <bitset>
#include <cstddef>
#include <optional>

class Engine : protected InputListener {
public:
    struct Mouse {
        std::optional<double> xpos;
        std::optional<double> ypos;
        /// Movement from the most recent cursor event, 0 if none this frame
        double xoffset{0.0};
        double yoffset{0.0};
        /// Scrolling accumulated over the current frame
        double scrollXOffset{0.0};
        double scrollYOffset{0.0};
    };

    class Keyboard {
    public:
        static constexpr std::size_t kMaxKeys = 1024;

        [[nodiscard]] std::bitset<kMaxKeys> const& keys() const {
            return keys_;
        }

        /// True if the key was pressed during the current frame, even if it
        /// has already been released
        [[nodiscard]] bool keyWasJustPressed(unsigned int key) const;

    private:
        friend class Engine;

        void onKey(int key, int action);
        void beginFrame() { justPressed_.reset(); }

        std::bitset<kMaxKeys> keys_;
        std::bitset<kMaxKeys> justPressed_;
    };

    [[nodiscard]] AssetLoader& assets() { return assets_; }

    [[nodiscard]] Keyboard const& keyboard() const { return keyboard_; }
    [[nodiscard]] Mouse const& mouse() const { return mouse_; }

protected:
    /// Load PAK archive
    /// @throws std::runtime_error on any error loading the PAK
    explicit Engine(CliArgs const& args);

    /// Reset per-frame input state. Call once per frame before polling
    /// events.
    void beginFrame();

    void onKey(int key, int /*scancode*/, int action, int /*mods*/) override;
    void onCursorPos(double xpos, double ypos) override;
    void onScroll(double xoffset, double yoffset) override;

    GlfwContext glfw_;
    Window window_;

    int width_{};
    int height_{};
    Keyboard keyboard_;
    Mouse mouse_;
    bool inputGoesToGame_{false};

    AssetLoader assets_;
};
