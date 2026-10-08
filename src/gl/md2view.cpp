#include "md2view/gl/md2view.hpp"
#include "md2view/gl/engine.hpp"
#include "md2view/ui/ui.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>

MD2View::MD2View() { reset_model_matrix(); }

void MD2View::load_model(GL::Engine<MD2View>& engine) {
    md2_ = engine.resource_manager().loadModel(model_selector_->model_path());
    md2_mesh_ = std::make_unique<GL::Mesh>(md2_->interpolated_vertices(),
                                           md2_->scaled_texcoords());
}

void MD2View::reset_model_matrix() {
    rot_[0] = 0.0f;
    rot_[1] = glm::radians(-90.0f); // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64;

    pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
}

void MD2View::reset_camera() { camera_.reset(glm::vec3(0.0f, 0.0f, 3.0f)); }

void MD2View::load_current_texture(GL::Engine<MD2View>& engine) {
    auto const& path = md2_->current_skin().fpath;
    texture_ = engine.resource_manager().load_texture2D(path);
}

bool MD2View::on_engine_initialized(GL::Engine<MD2View>& engine) {
    if (!engine.resource_manager().pak().has_models()) {
        // NB: converting filesystem path to string in format arg
        // to work-around clang-tidy issue from libfmt:
        // https://github.com/fmtlib/fmt/issues/4552
        spdlog::error("PAK '{}' has no MD2 models to view",
                      engine.resource_manager().pak().fpath().string());
        return false;
    }
    // init objects which needed an opengl context to initialize
    model_selector_ =
        std::make_unique<ModelSelector>(engine.resource_manager().pak());
    load_model(engine);

    spdlog::info("begin load shaders");
    shader_ = engine.resource_manager().load_shader("md2");
    shader_->use();
    update_model();
    load_current_texture(engine);
    glow_loc_ = shader_->uniform_location("glow_color");
    glow_color_ = glm::vec3(0.0f, 1.0f, 0.0f);
    GL::Shader::set_uniform(glow_loc_, glow_color_);
    glCheckError();
    camera_.set_position(glm::vec3(0.0f, 0.0f, 3.0f));
    spdlog::info("done on engine init");

    return true;
}

void MD2View::onFramebufferResize() { camera_.set_fov_dirty(); }

void MD2View::update_model() {
    // translate, rotate, scale
    model_ = glm::mat4(1.0);
    model_ = glm::translate(model_, pos_);
    glm::quat rotx = glm::angleAxis(rot_[0], glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat roty = glm::angleAxis(rot_[1], glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat rotz = glm::angleAxis(rot_[2], glm::vec3(0.0f, 0.0f, 1.0f));
    model_ *= glm::mat4_cast(roty * rotz * rotx);
    auto s = 1.0f / static_cast<float>(scale_); // uniform scale factor
    model_ = glm::scale(model_, glm::vec3(s, s, s));

    shader_->use();
    shader_->set_model(model_);
}

void MD2View::render(GL::Engine<MD2View>& engine) {
    shader_->use();

    if (camera_.view_dirty()) {
        view_ = camera_.view_matrix();
        shader_->set_view(view_);
        camera_.set_view_clean();
    }

    if (camera_.fov_dirty()) {
        projection_ =
            glm::perspective(glm::radians(camera_.fov()),
                             engine.renderer().aspectRatio(), 0.1f, 500.0f);

        shader_->set_projection(projection_);
        camera_.set_fov_clean();
    }

    // render normal frame
    texture_->bind();
    md2_mesh_->draw(*shader_);
    glCheckError();
    draw_ui(engine);
    glCheckError();
}

void MD2View::draw_ui(GL::Engine<MD2View>& engine) {
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
            reset_camera();
        }

        ImGui::Text("View");
        ImGui::PushItemWidth(vec4width);
        static std::array view_ids = {"view##0", "view##1", "view##2",
                                      "view##3"};

        for (auto i = 0U; i < view_ids.size(); ++i) {
            ImGui::PushID(gsl_lite::at(view_ids, i));
            auto const j = gsl_lite::narrow_cast<glm::length_t>(i);
            ImGui::InputFloat4("", glm::value_ptr(view_[j]), "%.3f",
                               ImGuiInputTextFlags_ReadOnly);
            ImGui::PopID();
        }
        ImGui::PopItemWidth();

        ImGui::Text("Projection");
        ImGui::PushItemWidth(vec4width);
        static std::array proj_ids = {"proj##0", "proj##1", "proj##2",
                                      "proj##3"};

        for (auto i = 0U; i < proj_ids.size(); ++i) {
            ImGui::PushID(gsl_lite::at(proj_ids, i));
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
                           model_selector_->model_path().c_str());

        if (UI::draw(*md2_)) {
            load_current_texture(engine);
        }

        ImGui::Text("Model");
        ImGui::PushItemWidth(vec4width);
        static std::array model_ids = {"model##00", "model##1", "model##2",
                                       "model##3"};

        for (auto i = 0U; i < model_ids.size(); ++i) {
            ImGui::PushID(gsl_lite::at(model_ids, i));
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
        if (ImGui::ColorEdit3("Glow color", glm::value_ptr(glow_color_))) {
            shader_->use();
            GL::Shader::set_uniform(glow_loc_, glow_color_);
        }

        bool model_changed = ImGui::SliderInt("Scale Factor", &scale_, 1, 256);
        model_changed |=
            ImGui::SliderFloat("X-Position", &pos_[0], -7.0f, 7.0f);
        model_changed |=
            ImGui::SliderFloat("Y-Position", &pos_[1], -7.0f, 7.0f);
        model_changed |=
            ImGui::SliderFloat("Z-Position", &pos_[2], -7.0f, 7.0f);
        model_changed |= ImGui::SliderAngle("X-Rotation", rot_.data());
        model_changed |= ImGui::SliderAngle("Y-Rotation", &rot_[1]);
        model_changed |= ImGui::SliderAngle("Z-Rotation", &rot_[2]);

        if (ImGui::Button("Reset Model")) {
            reset_model_matrix();
            model_changed = true;
        }
        if (model_changed) {
            update_model();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize,
                            std::max(1.0f, ImGui::GetStyle().ImageBorderSize));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(255, 255, 255, 128));
        ImGui::ImageWithBg(
            std::uintptr_t(texture_->id()),
            ImVec2(gsl_lite::narrow_cast<float>(texture_->width()),
                   gsl_lite::narrow_cast<float>(texture_->height())),
            ImVec2(0, 0), ImVec2(1, 1), ImVec4(255, 255, 255, 255));
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Select Model", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (model_selector_->draw_ui()) {
            load_model(engine);
            load_current_texture(engine); // skin may have changed
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void MD2View::update(GL::Engine<MD2View>& /* engine */, GLfloat delta_time) {
    md2_->update(delta_time);
    md2_mesh_->sync(md2_->interpolated_vertices());
}

void MD2View::process_input(GL::Engine<MD2View>& engine, GLfloat delta_time) {
    if (engine.keyboard().keys()[GLFW_KEY_W]) {
        camera_.move(Camera::Direction::FORWARD, delta_time);
    }
    if (engine.keyboard().keys()[GLFW_KEY_S]) {
        camera_.move(Camera::Direction::BACKWARD, delta_time);
    }
    if (engine.keyboard().keys()[GLFW_KEY_A]) {
        camera_.move(Camera::Direction::LEFT, delta_time);
    }
    if (engine.keyboard().keys()[GLFW_KEY_D]) {
        camera_.move(Camera::Direction::RIGHT, delta_time);
    }
}

void MD2View::on_mouse_movement(GLfloat xoffset, GLfloat yoffset) {
    camera_.on_mouse_movement(xoffset, yoffset);
}

void MD2View::on_mouse_scroll(double xoffset, double yoffset) {
    camera_.on_mouse_scroll(xoffset, yoffset);
}
