#include "md2view/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>

#include <iostream>

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
                         "PAK file or directory to emulate as a PAK");

    options_desc().add(engine);

    boost::program_options::store(boost::program_options::parse_command_line(
                                      args.size(), args.data(), options_desc()),
                                  variables_map_);
    boost::program_options::notify(variables_map_);

    if (variables_map_.count("help")) {
        std::cerr << options_desc() << '\n';
        return false;
    }

    return true;
}
