#include "md2view/engine.hpp"
#include "md2view/asset_loader.hpp"

#include <GLFW/glfw3.h>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
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

bool Engine::parseArgs(std::span<char const*> args) {
    boost::program_options::options_description engine("Engine options");
    engine.add_options()("help,h", "Show this help message")(
        "width,W",
        boost::program_options::value<int>(&width_)->default_value(1280),
        "Screen width")(
        "height,H",
        boost::program_options::value<int>(&height_)->default_value(800),
        "Screen height")("pak,p",
                         boost::program_options::value<std::string>(&pakPath_),
                         "PAK file or directory to emulate as a PAK")(
        "log-level,l",
        boost::program_options::value<std::string>()->default_value("info"),
        "Log level: debug, info, warn, error, off");

    optionsDesc().add(engine);

    boost::program_options::store(boost::program_options::parse_command_line(
                                      gsl_lite::narrow_cast<int>(args.size()),
                                      args.data(), optionsDesc()),
                                  variablesMap_);
    boost::program_options::notify(variablesMap_);

    if (variablesMap_.contains("help")) {
        std::cerr << optionsDesc() << '\n';
        return false;
    }

    static std::map<std::string, spdlog::level::level_enum> const levels = {
        {"debug", spdlog::level::debug}, {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},   {"error", spdlog::level::err},
        {"off", spdlog::level::off},
    };
    auto const& levelStr = variablesMap_["log-level"].as<std::string>();
    auto it = levels.find(levelStr);
    if (it == levels.end()) {
        std::cerr << "unknown log level '" << levelStr
                  << "'; choose: debug, info, warn, error, off\n";
        return false;
    }
    spdlog::set_level(it->second);

    return true;
}

bool Engine::init(std::span<char const*> args) {
    if (!parseArgs(args)) {
        return false;
    }

    std::optional<std::filesystem::path> pak;
    if (!pakPath_.empty()) {
        pak = pakPath_;
    }
    assets_ = std::make_unique<AssetLoader>("data", pak);

    doInit();
    window_.setInputListener(this);
    return true;
}

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
