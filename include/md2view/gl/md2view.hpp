#pragma once

#include "md2view/camera.hpp"
#include "md2view/gl/game.hpp"
#include "md2view/gl/texture2d.hpp"
#include "md2view/md2.hpp"
#include "md2view/renderer.hpp"
#include "md2view/ui/model_selector.hpp"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>

class MD2View : public GL::Game {
public:
    MD2View();

    [[nodiscard]] char const* title() const override { return "MD2View"; }
    [[nodiscard]] bool onEngineInitialized(GL::Engine& engine) override;
    void processInput(GL::Engine& engine, float deltaTime) override;
    void onMouseMovement(float xoffset, float yoffset) override;
    void onMouseScroll(double xoffset, double yoffset) override;
    void onFramebufferResize() override;
    void update(GL::Engine& engine, float deltaTime) override;
    void render(GL::Engine& engine) override;

private:
    void resetCamera();
    void resetModelMatrix();
    void loadCurrentTexture(GL::Engine& engine);
    void updateModel();
    void drawUi(GL::Engine& engine);
    void loadModel(GL::Engine& engine);

    std::shared_ptr<MD2> md2_;
    MeshHandle md2Mesh_;
    std::unique_ptr<ModelSelector> modelSelector_;
    std::shared_ptr<GL::Texture2D> texture_;

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
