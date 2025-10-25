#pragma once

#include "resource_manager.hpp"
#include "gui.hpp"

#include <boost/program_options.hpp>
#include <gsl/gsl-lite.hpp>

#include <bitset>
#include <random>
#include <memory>

class Engine
{
public:
    struct Mouse {
        std::optional<double> xpos;
        std::optional<double> ypos;
        std::optional<double> scroll_xoffset;
        std::optional<double> scroll_yoffset; 
    };

    static size_t const max_keys = 1024;

    Engine() = default;

    int width() const { return width_; }
    int height() const { return height_; }

    int screen_width() const { return screen_width_; }
    int screen_height() const { return screen_height_; }

    ResourceManager& resource_manager() 
    { 
        gsl_Expects(resource_manager_);
        return *resource_manager_; 
    }
    std::bitset<max_keys> const& keys() const { return keys_; }

    [[nodiscard]] bool check_key_pressed(unsigned int key);

    float aspect_ratio() const { return static_cast<float>(width_) / static_cast<float>(height_); }

    boost::program_options::options_description& options_desc() { return opt_desc_; }
    boost::program_options::variables_map const& variables_map() { return variables_map_; }

    Mouse const& mouse() const { return mouse_; }  

protected:
    bool parse_args(gsl::span<char const *> args);
    
    int width_;
    int height_;
    int screen_width_;
    int screen_height_;
    std::unique_ptr<ResourceManager> resource_manager_;

    std::bitset<max_keys> keys_;
    std::bitset<max_keys> keys_pressed_;
    Mouse mouse_;

    boost::program_options::options_description opt_desc_;
    boost::program_options::variables_map variables_map_;
    std::string pak_path_;
};

