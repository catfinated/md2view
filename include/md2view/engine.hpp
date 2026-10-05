#pragma once

#include "md2view/window.hpp"

#include <boost/program_options.hpp>

#include <bitset>
#include <cstddef>
#include <memory>
#include <span>

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

    Engine() = default;

    [[nodiscard]] bool init(std::span<char const*> args);

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    [[nodiscard]] int screen_width() const { return screen_width_; }
    [[nodiscard]] int screen_height() const { return screen_height_; }

    [[nodiscard]] float aspect_ratio() const {
        return static_cast<float>(width_) / static_cast<float>(height_);
    }

    boost::program_options::options_description& options_desc() {
        return opt_desc_;
    }
    boost::program_options::variables_map const& variables_map() {
        return variables_map_;
    }

    [[nodiscard]] Keyboard const& keyboard() const { return keyboard_; }
    [[nodiscard]] Mouse const& mouse() const { return mouse_; }

protected:
    bool parse_args(std::span<char const*> args);
    virtual void doInit() = 0;

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
    int screen_width_{};
    int screen_height_{};
    Keyboard keyboard_;
    Mouse mouse_;
    bool input_goes_to_game_{false};

    boost::program_options::options_description opt_desc_;
    boost::program_options::variables_map variables_map_;
    std::string pak_path_;
};
