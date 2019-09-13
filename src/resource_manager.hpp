#pragma once

#include "common.hpp"
#include "shader.hpp"
#include "texture2D.hpp"
#include "pcx.hpp"
#include "pak.hpp"

#include "stb_image.h"

#include <boost/filesystem.hpp>

#include <unordered_map>
#include <memory>

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

inline ResourceManager::ResourceManager(std::string const& rootdir)
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

inline std::shared_ptr<Shader> ResourceManager::load_shader(char const * vertex,
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

inline Texture2D& ResourceManager::load_texture2D(PAK const& pf, std::string const& path, std::string const& name)
{
    auto key = name.empty() ? path : name;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    std::cout << "loading 2D texture " << path << " from PAK " << pf.filename() << '\n';

    MD2V_EXPECT(".pcx" == boost::filesystem::path(path).extension());

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

    std::cout << "loaded 2D texture " << path << '\n';

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    MD2V_EXPECT(result.second);
    MD2V_EXPECT(!texture.initialized());
    MD2V_EXPECT(result.first->second.initialized());

    return result.first->second;
}

inline Texture2D& ResourceManager::load_texture2D(std::string const& file, std::string const& name, bool alpha)
{
    auto key = name.empty() ? file : name;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    boost::filesystem::path p(file);
    auto texture_path = p.string();

    MD2V_EXPECT(boost::filesystem::exists(p));

    std::cout << "loading 2D texture " << texture_path << " (alpha: " << std::boolalpha << alpha << ")\n";

    Texture2D texture;

    if (".pcx" != p.extension()) {
        texture.set_alpha(alpha);

        int width, height, n;
        unsigned char * image = stbi_load(texture_path.c_str(), &width, &height, &n, 3);
        MD2V_EXPECT(image);
        MD2V_EXPECT(texture.init(width, height, image));
        std::cout << "loaded 2D texture " << texture_path << " width: " << width << " height: " << height << "\n";
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
