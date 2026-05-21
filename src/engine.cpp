#include "md2view/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <map>

bool Engine::check_key_pressed(unsigned int key) {
    gsl_Expects(key < max_keys);

    if (keys_[key] && !keys_pressed_[key]) {
        keys_pressed_[key] = true;
        return true;
    }
    return false;
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
