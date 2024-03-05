#pragma once

#include "shader.hpp"
#include "texture2D.hpp"
#include "pak.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class ResourceManager
{
public:
    explicit ResourceManager(std::filesystem::path const& rootdir);

    // accessors
    std::filesystem::path const& root_dir() const { return root_dir_; }
    std::filesystem::path const& models_dir() const { return models_dir_; }
    std::filesystem::path const& shaders_dir() const { return shaders_dir_; }

    std::shared_ptr<Shader> load_shader(std::string const& name,
                                        std::string_view vertex,
                                        std::string_view fragment,
                                        std::optional<std::string_view> geometry = {});
                                        
    std::shared_ptr<Shader> shader(std::string const& name) { return shaders_.at(name); }

    std::shared_ptr<Texture2D> load_texture2D(std::filesystem::path const& path, std::optional<std::string> const& name = {}, bool alpha = false);

    std::shared_ptr<Texture2D> load_texture2D(PAK const& pf, std::string const& path, std::optional<std::string> const& name = {});

    std::shared_ptr<Texture2D> texture2D(std::string const& name) { return textures2D_.at(name); }

private:
    std::filesystem::path root_dir_;
    std::filesystem::path models_dir_;
    std::filesystem::path shaders_dir_;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textures2D_;
};
