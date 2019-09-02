#pragma once

#include "common.hpp"
#include "shader.hpp"
#include "texture2D.hpp"
#include "pcx.hpp"
#include "pak.hpp"

#include <boost/filesystem.hpp>

#include <SOIL.h>

#include <unordered_map>

namespace blue {

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
    std::string const& root_dir()     const { return root_dir_;     }
    std::string const& models_dir()   const { return models_dir_;   }
    std::string const& textures_dir() const { return textures_dir_; }
    std::string const& shaders_dir()  const { return shaders_dir_;  }
    std::string const& audio_dir()    const { return audio_dir_;    }
    std::string const& fonts_dir()    const { return fonts_dir_;    }

    Shader& load_shader(char const * vertex,
                        char const * fragment,
                        char const * geometry,
                        std::string const& name);

    Shader& shader(std::string const& name) { return shaders_.at(name); }

    Texture2D& load_texture2D(std::string const& path, std::string const& name = std::string(), bool alpha = false);

    Texture2D& load_texture2D(PakFile const& pf, std::string const& path, std::string const& name = std::string());

    Texture2D& texture2D(std::string const& name) { return textures2D_.at(name); }

private:
    std::string root_dir_;
    std::string models_dir_;
    std::string textures_dir_;
    std::string shaders_dir_;
    std::string audio_dir_;
    std::string fonts_dir_;

    std::unordered_map<std::string, Shader> shaders_;
    std::unordered_map<std::string, Texture2D> textures2D_;
};

inline ResourceManager::ResourceManager(std::string const& rootdir)
    : root_dir_(rootdir)
    , models_dir_("models/")
    , textures_dir_("textures/")
    , shaders_dir_("shaders/")
    , audio_dir_("audio/")
    , fonts_dir_("fonts/")
{
    if (!root_dir_.empty()) {
        if (root_dir_.back() != '/') {
            root_dir_ += '/';
        }

        models_dir_   = root_dir_ + models_dir_;
        textures_dir_ = root_dir_ + textures_dir_;
        shaders_dir_  = root_dir_ + shaders_dir_;
        audio_dir_    = root_dir_ + audio_dir_;
        fonts_dir_    = root_dir_ + fonts_dir_;
    }
}

inline Shader& ResourceManager::load_shader(char const * vertex,
                                            char const * fragment,
                                            char const * geometry,
                                            std::string const& name)
{
    BLUE_EXPECT(vertex != nullptr);
    BLUE_EXPECT(fragment != nullptr);
    BLUE_EXPECT(shaders_.find(name) == shaders_.end());

    auto vertex_path = shaders_dir() + vertex;
    auto fragment_path = shaders_dir() + fragment;
    auto geometry_path = geometry != nullptr ? shaders_dir() + geometry : std::string{};

    Shader shader{vertex_path, fragment_path, geometry_path};

    //auto result = shaders_.emplace(std::piecewise_construct,
    //                               std::forward_as_tuple(name),
    //                               std::forward_as_tuple(vertex_path, fragment_path, geometry_path));

    auto result = shaders_.emplace(std::make_pair(name, std::move(shader)));

    BLUE_EXPECT(result.second);
    BLUE_EXPECT(!shader.initialized());
    BLUE_EXPECT(result.first->second.initialized());

    return result.first->second;
}

inline Texture2D& ResourceManager::load_texture2D(PakFile const& pf, std::string const& path, std::string const& name)
{
    auto key = name.empty() ? path : name;
    auto iter = textures2D_.find(key);

    if (iter != textures2D_.end()) {
        return iter->second;
    }

    std::cout << "loading 2D texture " << path << " from PAK " << pf.filename() << '\n';

    BLUE_EXPECT(".pcx" == boost::filesystem::path(path).extension());

    auto node = pf.find(path);
    BLUE_EXPECT(node);

    std::ifstream inf(pf.filename().c_str(), std::ios_base::in | std::ios_base::binary);
    BLUE_EXPECT(inf);

    inf.seekg(node->filepos);

    PcxFile pcx(inf);
    inf.close();

    Texture2D texture;
    texture.set_alpha(false);
    texture.init(pcx.width(), pcx.height(), pcx.image().data());

    std::cout << "loaded 2D texture " << path << '\n';

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    BLUE_EXPECT(result.second);
    BLUE_EXPECT(!texture.initialized());
    BLUE_EXPECT(result.first->second.initialized());

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

    if (!boost::filesystem::exists(p)) {
        p = boost::filesystem::path(textures_dir()) / p;
        texture_path = p.string();
    }

    std::cout << "loading 2D texture " << texture_path << " (alpha: " << std::boolalpha << alpha << ")\n";

    Texture2D texture;

    if (".pcx" != p.extension()) {
        texture.set_alpha(alpha);

        int width, height;
        unsigned char * image = SOIL_load_image(texture_path.c_str(),
                                                &width,
                                                &height,
                                                0,
                                                alpha ? SOIL_LOAD_RGBA : SOIL_LOAD_RGB);

        BLUE_EXPECT(image);
        BLUE_EXPECT(texture.init(width, height, image));
        std::cout << "loaded 2D texture " << texture_path << " width: " << width << " height: " << height << "\n";

        SOIL_free_image_data(image);
    }
    else {
        std::ifstream inf(p.string().c_str(), std::ios_base::in | std::ios_base::binary);
        PcxFile pcx(inf);
        texture.set_alpha(false);
        texture.init(pcx.width(), pcx.height(), pcx.image().data());
        inf.close();
    }

    auto result = textures2D_.emplace(std::make_pair(key, std::move(texture)));

    BLUE_EXPECT(result.second);
    BLUE_EXPECT(!texture.initialized());
    BLUE_EXPECT(result.first->second.initialized());

    return result.first->second;
}

} // namespace blue