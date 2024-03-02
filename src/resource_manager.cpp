#include "resource_manager.hpp"
#include "common.hpp"
#include "pcx.hpp"

#include <spdlog/spdlog.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>

ResourceManager::ResourceManager(std::string const& rootdir)
    : root_dir_(rootdir)
    , models_dir_("models/")
    , shaders_dir_("shaders/")
{
    if (!root_dir_.empty()) {
        if (root_dir_.back() != '/') {
            root_dir_ += '/';
        }

        models_dir_   = root_dir_ + models_dir_;
        shaders_dir_  = root_dir_ + shaders_dir_;
    }
}

std::shared_ptr<Shader> ResourceManager::load_shader(char const * vertex,
                                                    char const * fragment,
                                                    char const * geometry,
                                                    std::string const& name)
{
    MD2V_EXPECT(vertex != nullptr);
    MD2V_EXPECT(fragment != nullptr);
    MD2V_EXPECT(shaders_.find(name) == shaders_.end());

    auto vertex_path = shaders_dir() + vertex;
    auto fragment_path = shaders_dir() + fragment;
    auto geometry_path = geometry != nullptr ? shaders_dir() + geometry : std::string{};

    std::shared_ptr<Shader> shader{new Shader{vertex_path, fragment_path, geometry_path}};
    auto result = shaders_.emplace(std::make_pair(name, shader));

    MD2V_EXPECT(result.second);
    MD2V_EXPECT(shader->initialized());
    MD2V_EXPECT(result.first->second->initialized());

    return result.first->second;
}

Texture2D& ResourceManager::load_texture2D(PAK const& pf, std::string const& path, std::string const& name)
{
    auto key = name.empty() ? path : name;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    spdlog::info("loading 2D texture {} from PAK {}", path, pf.filename());

    MD2V_EXPECT(".pcx" == std::filesystem::path(path).extension());

    auto node = pf.find(path);
    MD2V_EXPECT(node);

    std::ifstream inf(pf.filename().c_str(), std::ios_base::in | std::ios_base::binary);
    MD2V_EXPECT(inf);

    inf.seekg(node->filepos);

    PCX pcx(inf);
    inf.close();

    Texture2D texture;
    texture.set_alpha(false);
    texture.init(pcx.width(), pcx.height(), pcx.image().data());

    spdlog::info("loaded 2D texture {}", path);

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    MD2V_EXPECT(result.second);
    MD2V_EXPECT(!texture.initialized());
    MD2V_EXPECT(result.first->second.initialized());

    return result.first->second;
}

Texture2D& ResourceManager::load_texture2D(std::string const& file, std::string const& name, bool alpha)
{
    auto key = name.empty() ? file : name;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    std::filesystem::path p(file);
    auto texture_path = p.string();

    MD2V_EXPECT(std::filesystem::exists(p));

    spdlog::info("loading 2D texture {} (alpha: {})", texture_path, alpha);

    Texture2D texture;

    if (".pcx" != p.extension()) {
        texture.set_alpha(alpha);

        int width, height, n;
        unsigned char * image = stbi_load(texture_path.c_str(), &width, &height, &n, 3);
        MD2V_EXPECT(image);
        MD2V_EXPECT(texture.init(width, height, image));
        spdlog::info("loaded 2D texture {} width: {} height: {}", texture_path, width, height);
        stbi_image_free(image);
    }
    else {
        std::ifstream inf(p.string().c_str(), std::ios_base::in | std::ios_base::binary);
        PCX pcx(inf);
        texture.set_alpha(false);
        texture.init(pcx.width(), pcx.height(), pcx.image().data());
        inf.close();
    }

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    MD2V_EXPECT(result.second);
    MD2V_EXPECT(!texture.initialized());
    MD2V_EXPECT(result.first->second.initialized());

    return result.first->second;
}
