/// @file
/// MD2 model viewer, the Game run by each engine
#pragma once

#include "md2view/camera.hpp"
#include "md2view/game.hpp"
#include "md2view/md2.hpp"
#include "md2view/renderer.hpp"
#include "md2view/ui/model_selector.hpp"

#include "md2view/detail/string_hash.hpp"

#include <glm/glm.hpp>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class MD2View : public Game {
public:
    MD2View();

    [[nodiscard]] char const* title() const override { return "MD2View"; }
    [[nodiscard]] bool onEngineInitialized(Engine& engine) override;
    void processInput(Engine& engine, float deltaTime) override;
    void onMouseMovement(float xoffset, float yoffset) override;
    void onMouseScroll(double xoffset, double yoffset) override;
    void onFramebufferResize() override;
    void update(Engine& engine, float deltaTime) override;
    void render(Engine& engine) override;

private:
    void resetCamera();
    void resetModelMatrix();
    void loadCurrentTexture(Engine& engine);
    void updateModel();
    void drawUi(Engine& engine);
    void loadModel(Engine& engine);

    std::shared_ptr<MD2> md2_;
    std::unique_ptr<ModelSelector> modelSelector_;
    MeshHandle md2Mesh_;
    TextureHandle texture_;
    std::unordered_map<std::string,
                       TextureHandle,
                       detail::StringHash,
                       std::equal_to<>>
        textures_;

    Camera camera_;
    std::string modelsDir_;
    int scale_ = 64.0f;
    std::array<float, 3> rot_{};
    glm::vec3 pos_{};
    glm::mat4 model_{};
    glm::mat4 view_{};
    glm::mat4 projection_{};
    glm::vec3 glowColor_{0.0f, 1.0f, 0.0f};
};
