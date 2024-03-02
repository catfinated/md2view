#pragma once

#include "shader.hpp"
#include "texture2D.hpp"
#include "pak.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class ResourceManager
{
public:
    explicit ResourceManager(std::string const& rootdir);

    // non-copyable
    ResourceManager(ResourceManager const&) = delete;
    ResourceManager& operator=(ResourceManager const&) = delete;

    // non-movable
    ResourceManager(ResourceManager&&) = delete;
    ResourceManager& operator=(ResourceManager&&) = delete;

    // accessors
    std::string const& root_dir() const { return root_dir_; }
    std::string const& models_dir() const { return models_dir_; }
    std::string const& shaders_dir() const { return shaders_dir_; }

    std::shared_ptr<Shader> load_shader(char const * vertex,
                                        char const * fragment,
                                        char const * geometry,
                                        std::string const& name);

    std::shared_ptr<Shader> shader(std::string const& name) { return shaders_.at(name); }

    Texture2D& load_texture2D(std::string const& path, std::string const& name = std::string(), bool alpha = false);

    Texture2D& load_texture2D(PAK const& pf, std::string const& path, std::string const& name = std::string());

    Texture2D& texture2D(std::string const& name) { return textures2D_.at(name); }

private:
    std::string root_dir_;
    std::string models_dir_;
    std::string shaders_dir_;

    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
    std::unordered_map<std::string, Texture2D> textures2D_;
};
