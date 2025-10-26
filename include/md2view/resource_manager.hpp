#pragma once

#include "md2.hpp"
#include "pak.hpp"
#include "shader.hpp"
#include "texture2D.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

class ResourceManager {
public:
    explicit ResourceManager(
        std::filesystem::path const& rootdir,
        std::optional<std::filesystem::path> const& pak_path = std::nullopt);

    // accessors
    std::filesystem::path const& root_dir() const { return root_dir_; }
    std::filesystem::path const& shaders_dir() const { return shaders_dir_; }
    PAK& pak() {
        gsl_Assert(pak_);
        return *pak_;
    }

    std::shared_ptr<Shader>
    load_shader(std::string const& name,
                std::optional<std::string_view> vertex = {},
                std::optional<std::string_view> fragment = {},
                std::optional<std::string_view> geometry = {});

    std::shared_ptr<Shader> shader(std::string const& name) {
        return shaders_.at(name);
    }

    std::shared_ptr<Texture2D>
    load_texture2D(std::string const& path,
                   std::optional<std::string> const& name = {});

    std::shared_ptr<Texture2D> texture2D(std::string const& name) {
        return textures2D_.at(name);
    }

    std::shared_ptr<MD2> load_model(std::string const& path);

private:
    std::filesystem::path root_dir_;
    std::filesystem::path shaders_dir_;
    std::unique_ptr<PAK> pak_;
    std::unordered_map<std::string, std::shared_ptr<Shader>> shaders_;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> textures2D_;
    std::unordered_map<std::string, std::shared_ptr<MD2>> models_;
};
