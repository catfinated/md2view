#pragma once

#include "md2view/camera.hpp"
#include "md2view/gl/gl.hpp"
#include "md2view/gl/mesh.hpp"
#include "md2view/gl/texture2d.hpp"
#include "md2view/md2.hpp"
#include "md2view/ui/model_selector.hpp"

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <string>

namespace GL {
template <typename Game> class Engine;
}

class MD2View {
public:
    MD2View();

    bool onEngineInitialized(GL::Engine<MD2View>& engine);
    void processInput(GL::Engine<MD2View>& engine, GLfloat deltaTime);
    void onMouseMovement(GLfloat xoffset, GLfloat yoffset);
    void onMouseScroll(double xoffset, double yoffset);
    void onFramebufferResize();
    void update(GL::Engine<MD2View>& engine, GLfloat deltaTime);
    void render(GL::Engine<MD2View>& engine);
    static char const* title() { return "MD2View"; }

private:
    void resetCamera();
    void resetModelMatrix();
    void loadCurrentTexture(GL::Engine<MD2View>& engine);
    void updateModel();
    void drawUi(GL::Engine<MD2View>& engine);
    void loadModel(GL::Engine<MD2View>& engine);

    std::shared_ptr<MD2> md2_;
    std::unique_ptr<GL::Mesh> md2Mesh_;
    std::unique_ptr<ModelSelector> modelSelector_;
    std::shared_ptr<GL::Texture2D> texture_;
    std::shared_ptr<GL::Shader> shader_;

    Camera camera_;
    std::string modelsDir_;
    int scale_ = 64.0f;
    std::array<float, 3> rot_{};
    glm::vec3 pos_{};
    glm::mat4 model_{};
    glm::mat4 view_{};
    glm::mat4 projection_{};
    glm::vec3 glowColor_{};
    GLint glowLoc_{};
};
