#include "md2view/md2view.hpp"
#include "md2view/engine.hpp"
#include "md2view/renderer.hpp"
#include "md2view/ui/model_selector.hpp"
#include "md2view/ui/ui.hpp"

#include <GLFW/glfw3.h>
#include <fmt/std.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <memory>

MD2View::MD2View() { resetModelMatrix(); }

void MD2View::loadModel(Engine& engine) {
    md2_ = engine.assets().loadModel(modelSelector_->modelPath());
    auto& renderer = engine.renderer();
    if (md2Mesh_ != MeshHandle{}) {
        renderer.destroyMesh(md2Mesh_);
    }
    md2Mesh_ = renderer.createMesh(md2_->interpolatedVertices(),
                                   md2_->scaledTexcoords());
}

void MD2View::resetModelMatrix() {
    rot_[0] = 0.0f;
    rot_[1] = glm::radians(-90.0f); // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64;

    pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
}

void MD2View::resetCamera() { camera_.reset(glm::vec3(0.0f, 0.0f, 3.0f)); }

void MD2View::loadCurrentTexture(Engine& engine) {
    auto const& path = md2_->currentSkin().fpath;
    auto const it = textures_.find(path);
    if (it != textures_.end()) {
        texture_ = it->second;
        return;
    }

    texture_ = engine.renderer().createTexture(engine.assets().loadImage(path));
    textures_.emplace(path, texture_);
}

bool MD2View::onEngineInitialized(Engine& engine) {
    if (!engine.assets().pak().hasModels()) {
        spdlog::error("PAK '{}' has no MD2 models to view",
                      engine.assets().pak().fpath());
        return false;
    }
    // init objects which needed an opengl context to initialize
    modelSelector_ = std::make_unique<ModelSelector>(engine.assets().pak());
    loadModel(engine);
    updateModel();
    loadCurrentTexture(engine);
    camera_.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    spdlog::info("done on engine init");

    return true;
}

void MD2View::onFramebufferResize() { camera_.setFovDirty(); }

void MD2View::updateModel() {
    // translate, rotate, scale
    model_ = glm::mat4(1.0);
    model_ = glm::translate(model_, pos_);
    glm::quat rotx = glm::angleAxis(rot_[0], glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat roty = glm::angleAxis(rot_[1], glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat rotz = glm::angleAxis(rot_[2], glm::vec3(0.0f, 0.0f, 1.0f));
    model_ *= glm::mat4_cast(roty * rotz * rotx);
    auto s = 1.0f / static_cast<float>(scale_); // uniform scale factor
    model_ = glm::scale(model_, glm::vec3(s, s, s));
}

void MD2View::render(Engine& engine) {
    if (camera_.viewDirty()) {
        view_ = camera_.viewMatrix();
        camera_.setViewClean();
    }

    if (camera_.fovDirty()) {
        projection_ =
            glm::perspective(glm::radians(camera_.fov()),
                             engine.renderer().aspectRatio(), 0.1f, 500.0f);
        camera_.setFovClean();
    }

    engine.renderer().setCamera(view_, projection_);
    engine.renderer().drawModel(md2Mesh_, texture_, model_, glowColor_);
    drawUi(engine);
}

void MD2View::drawUi(Engine& engine) {
    static float const vec4width = 275;
    // draw gui
    ImGui::Begin("MD2View");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    auto clearColor = engine.renderer().clearColor();
    if (ImGui::ColorEdit3("Clear color", clearColor.data())) {
        engine.renderer().setClearColor(clearColor);
    }

    bool vsyncEnabled{engine.renderer().vsyncOn()};
    if (ImGui::Checkbox("V-sync", &vsyncEnabled)) {
        engine.renderer().setVsyncOn(vsyncEnabled);
    }

    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {

        UI::draw(camera_);

        if (ImGui::Button("Reset Camera")) {
            resetCamera();
        }

        ImGui::Text("View");
        ImGui::PushItemWidth(vec4width);
        static std::array viewIds = {
            "view##0",
            "view##1",
            "view##2",
            "view##3",
        };

        for (auto i = 0U; i < viewIds.size(); ++i) {
            ImGui::PushID(gsl_lite::at(viewIds, i));
            auto const j = gsl_lite::narrow_cast<glm::length_t>(i);
            ImGui::InputFloat4("", glm::value_ptr(view_[j]), "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
        }
        ImGui::PopItemWidth();

        ImGui::Text("Projection");
        ImGui::PushItemWidth(vec4width);
        static std::array projIds = {
            "proj##0",
            "proj##1",
            "proj##2",
            "proj##3",
        };

        for (auto i = 0U; i < projIds.size(); ++i) {
            ImGui::PushID(gsl_lite::at(projIds, i));
            auto const j = gsl_lite::narrow_cast<glm::length_t>(i);
            ImGui::InputFloat4("", glm::value_ptr(projection_[j]), "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
        }
        ImGui::PopItemWidth();

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Model: %s",
                           modelSelector_->modelPath().c_str());

        if (UI::draw(*md2_)) {
            loadCurrentTexture(engine);
        }

        ImGui::Text("Model");
        ImGui::PushItemWidth(vec4width);
        static std::array modelIds = {
            "model##00",
            "model##1",
            "model##2",
            "model##3",
        };

        for (auto i = 0U; i < modelIds.size(); ++i) {
            ImGui::PushID(gsl_lite::at(modelIds, i));
            auto const j = gsl_lite::narrow_cast<glm::length_t>(i);
            ImGui::InputFloat4("", glm::value_ptr(model_[j]), "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
        }
        ImGui::PopItemWidth();

        bool glow{engine.renderer().glowOn()};
        if (ImGui::Checkbox("Glow", &glow)) {
            engine.renderer().setGlowOn(glow);
        }
        ImGui::ColorEdit3("Glow color", glm::value_ptr(glowColor_));

        bool modelChanged = ImGui::SliderInt("Scale Factor", &scale_, 1, 256);
        modelChanged |= ImGui::SliderFloat("X-Position", &pos_[0], -7.0f, 7.0f);
        modelChanged |= ImGui::SliderFloat("Y-Position", &pos_[1], -7.0f, 7.0f);
        modelChanged |= ImGui::SliderFloat("Z-Position", &pos_[2], -7.0f, 7.0f);
        modelChanged |= ImGui::SliderAngle("X-Rotation", rot_.data());
        modelChanged |= ImGui::SliderAngle("Y-Rotation", &rot_[1]);
        modelChanged |= ImGui::SliderAngle("Z-Rotation", &rot_[2]);

        if (ImGui::Button("Reset Model")) {
            resetModelMatrix();
            modelChanged = true;
        }
        if (modelChanged) {
            updateModel();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize,
                            std::max(1.0f, ImGui::GetStyle().ImageBorderSize));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(255, 255, 255, 128));
        auto const skinSize = engine.renderer().textureSize(texture_);
        ImGui::ImageWithBg(
            static_cast<ImTextureID>(engine.renderer().imguiTexture(texture_)),
            ImVec2(gsl_lite::narrow_cast<float>(skinSize.x),
                   gsl_lite::narrow_cast<float>(skinSize.y)),
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(255, 255, 255, 255));
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Select Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (modelSelector_->drawUi()) {
            loadModel(engine);
            loadCurrentTexture(engine); // skin may have changed
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void MD2View::update(Engine& engine, float deltaTime) {
    md2_->update(deltaTime);
    engine.renderer().updateMesh(md2Mesh_, md2_->interpolatedVertices());
}

void MD2View::processInput(Engine& engine, float deltaTime) {
    if (engine.keyboard().keys()[GLFW_KEY_W]) {
        camera_.move(Camera::Direction::FORWARD, deltaTime);
    }
    if (engine.keyboard().keys()[GLFW_KEY_S]) {
        camera_.move(Camera::Direction::BACKWARD, deltaTime);
    }
    if (engine.keyboard().keys()[GLFW_KEY_A]) {
        camera_.move(Camera::Direction::LEFT, deltaTime);
    }
    if (engine.keyboard().keys()[GLFW_KEY_D]) {
        camera_.move(Camera::Direction::RIGHT, deltaTime);
    }
}

void MD2View::onMouseMovement(float xoffset, float yoffset) {
    camera_.onMouseMovement(xoffset, yoffset);
}

void MD2View::onMouseScroll(double xoffset, double yoffset) {
    camera_.onMouseScroll(xoffset, yoffset);
}
