#include "md2view/engine.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <map>
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

bool Engine::parse_args(std::span<char const*> args) {
    boost::program_options::options_description engine("Engine options");
    engine.add_options()("help,h", "Show this help message")(
        "width,W",
        boost::program_options::value<int>(&width_)->default_value(1280),
        "Screen width")(
        "height,H",
        boost::program_options::value<int>(&height_)->default_value(800),
        "Screen height")("pak,p",
                         boost::program_options::value<std::string>(&pak_path_),
                         "PAK file or directory to emulate as a PAK")(
        "log-level,l",
        boost::program_options::value<std::string>()->default_value("info"),
        "Log level: debug, info, warn, error, off");

    options_desc().add(engine);

    boost::program_options::store(boost::program_options::parse_command_line(
                                      gsl_lite::narrow_cast<int>(args.size()),
                                      args.data(), options_desc()),
                                  variables_map_);
    boost::program_options::notify(variables_map_);

    if (variables_map_.contains("help")) {
        std::cerr << options_desc() << '\n';
        return false;
    }

    static std::map<std::string, spdlog::level::level_enum> const levels = {
        {"debug", spdlog::level::debug}, {"info", spdlog::level::info},
        {"warn", spdlog::level::warn},   {"error", spdlog::level::err},
        {"off", spdlog::level::off},
    };
    auto const& level_str = variables_map_["log-level"].as<std::string>();
    auto it = levels.find(level_str);
    if (it == levels.end()) {
        std::cerr << "unknown log level '" << level_str
                  << "'; choose: debug, info, warn, error, off\n";
        return false;
    }
    spdlog::set_level(it->second);

    return true;
}

bool Engine::init(std::span<char const*> args) {
    if (!parse_args(args)) {
        return false;
    }

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
        input_goes_to_game_ = !input_goes_to_game_;
        spdlog::info("got F1. game input: {}", input_goes_to_game_);
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
