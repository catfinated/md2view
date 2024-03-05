#include "resource_manager.hpp"
#include "common.hpp"
#include "pcx.hpp"

#include <spdlog/spdlog.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <fstream>

ResourceManager::ResourceManager(std::filesystem::path const& rootdir)
    : root_dir_(rootdir)
    , models_dir_(root_dir_ / "models")
    , shaders_dir_(root_dir_ / "shaders")
{
}

std::shared_ptr<Shader> ResourceManager::load_shader(std::string const& name,
                                                     std::string_view vertex,
                                                     std::string_view fragment,
                                                     std::optional<std::string_view> geometry)
{
    spdlog::info("loading shader {}", name);
    MD2V_EXPECT(shaders_.find(name) == shaders_.end());

    auto vertex_path = shaders_dir() / std::string{vertex};
    auto fragment_path = shaders_dir() / std::string{fragment};
    std::optional<std::filesystem::path> geometry_path;
    if (geometry) {
        geometry_path = shaders_dir() / std::string{*geometry};
    }

    std::shared_ptr<Shader> shader{new Shader{vertex_path, fragment_path, geometry_path}};
    auto result = shaders_.emplace(std::make_pair(name, shader));

    MD2V_EXPECT(result.second);
    spdlog::info("loaded shader {}", name);
    return result.first->second;
}

std::shared_ptr<Texture2D> ResourceManager::load_texture2D(PAK const& pf, std::string const& path, std::optional<std::string> const& name)
{
    if (pf.is_directory()) {
        return load_texture2D(path, name);
    }
    auto key = name ? *name : path;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    spdlog::info("loading 2D texture {} from PAK {} ({})", path, pf.fpath().string(), name.value_or(std::string{}));
    MD2V_EXPECT(".pcx" == std::filesystem::path(path).extension());

    auto const& node = pf.entries().at(path);
    std::ifstream inf(pf.fpath(), std::ios_base::in | std::ios_base::binary);
    MD2V_EXPECT(inf);

    inf.seekg(node.filepos);

    PCX pcx(inf);
    inf.close();

    auto texture = std::make_shared<Texture2D>(pcx.width(), pcx.height(), pcx.image().data());;
    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    MD2V_EXPECT(result.second);
    return result.first->second;
}

std::shared_ptr<Texture2D> ResourceManager::load_texture2D(std::filesystem::path const& fpath, std::optional<std::string> const& name, bool alpha)
{
    auto key = name ? *name : fpath.string();
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    spdlog::info("loading 2D texture {} (alpha: {} name: {})", fpath.string(), alpha, name.value_or(std::string{}));
    MD2V_EXPECT(std::filesystem::exists(fpath));

    std::shared_ptr<Texture2D> texture;

    if (".pcx" != fpath.extension()) {
        int width, height, n;
        unsigned char * image = stbi_load(fpath.string().c_str(), &width, &height, &n, 3);
        MD2V_EXPECT(image);
        texture = std::make_shared<Texture2D>(width, height, image, alpha);
        spdlog::info("loaded 2D texture {} width: {} height: {}", fpath.string(), width, height);
        stbi_image_free(image);
    }
    else {
        std::ifstream inf(fpath, std::ios_base::in | std::ios_base::binary);
        PCX pcx(inf);
        texture = std::make_shared<Texture2D>(pcx.width(), pcx.height(), pcx.image().data());
        inf.close();
    }

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));
    MD2V_EXPECT(result.second);
    return result.first->second;
}
