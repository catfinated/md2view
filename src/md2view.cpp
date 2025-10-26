#include "md2view.hpp"
#include "engine.hpp"

#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

MD2View::MD2View() { reset_model_matrix(); }

void MD2View::load_model(Engine& engine) {
    md2_ = engine.resource_manager().load_model(model_selector_->model_path());
}

void MD2View::reset_model_matrix() {
    rot_[0] = 0.0f;
    rot_[1] = glm::radians(-90.0f); // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64;

    pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
}

void MD2View::reset_camera() { camera_.reset(glm::vec3(0.0f, 0.0f, 3.0f)); }

void MD2View::load_current_texture(Engine& engine) {
    auto const& path = md2_->current_skin().fpath;
    texture_ = engine.resource_manager().load_texture2D(path);
}

bool MD2View::on_engine_initialized(Engine& engine) {
    if (!engine.resource_manager().pak().has_models()) {
        // NB: converting filesystem path to string in format arg
        // to work-around clang-tidy issue from libfmt:
        // implicit instantiation of undefined template 'fmt::detail::type_is_unformattable_for<std::filesystem::path, char>' [clang-diagnostic-error]
        spdlog::error("PAK '{}' has no MD2 models to view",
                      engine.resource_manager().pak().fpath().string());
        return false;
    }
    // init objects which needed an opengl context to initialize
    model_selector_ =
        std::make_unique<ModelSelector>(engine.resource_manager().pak());
    load_model(engine);
    blur_fb_ = std::make_unique<FrameBuffer>(engine.width(), engine.height(), 1,
                                             false);
    main_fb_ =
        std::make_unique<FrameBuffer>(engine.width(), engine.height(), 2, true);
    screen_quad_ = std::make_unique<ScreenQuad>();

    clear_color_ = {0.2f, 0.2f, 0.2f, 1.0f};
    glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], 1.0f);

    spdlog::info("begin load shaders");
    shader_ = engine.resource_manager().load_shader("md2");
    shader_->use();
    update_model();
    load_current_texture(engine);
    glow_loc_ = shader_->uniform_location("glow_color");
    glow_color_ = glm::vec3(0.0f, 1.0f, 0.0f);
    shader_->set_uniform(glow_loc_, glow_color_);

    blur_shader_ = engine.resource_manager().load_shader("blur", "screen");
    blur_shader_->use();
    disable_blur_loc_ = blur_shader_->uniform_location("disable_blur");
    blur_shader_->set_uniform(disable_blur_loc_, 1);

    glow_shader_ = engine.resource_manager().load_shader("glow", "screen");
    glow_shader_->use();

    auto loc = glow_shader_->uniform_location("screenTexture");
    glow_shader_->set_uniform(loc, 0);
    loc = glow_shader_->uniform_location("prepassTexture");
    glow_shader_->set_uniform(loc, 1);
    loc = glow_shader_->uniform_location("blurredTexture");
    glow_shader_->set_uniform(loc, 2);

    camera_.set_position(glm::vec3(0.0f, 0.0f, 3.0f));

    main_fb_->bind();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);
    // glDisable(GL_BLEND);
    glEnable(GL_BLEND);
    FrameBuffer::bind_default();

    set_vsync();
    spdlog::info("done on engine init");
    glCheckError();
    return true;
}

void MD2View::on_framebuffer_resized(int width, int height) {
    glm::mat4 projection = glm::perspective(
        glm::radians(camera_.fov()),
        static_cast<float>(width) / static_cast<float>(height), 0.1f, 500.0f);

    shader_->use();
    shader_->set_projection(projection);

    FrameBuffer::bind_default();
    glViewport(0, 0, width, height);

    main_fb_ = std::make_unique<FrameBuffer>(width, height, 2, true);
    blur_fb_ = std::make_unique<FrameBuffer>(width, height, 1, false);
}

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

void MD2View::render(Engine& engine) {
    // ImGui::ShowTestWindow(nullptr);
    // return;
    shader_->use();

    if (camera_.view_dirty()) {
        view_ = camera_.view_matrix();
        shader_->set_view(view_);
        camera_.set_view_clean();
    }

    if (camera_.fov_dirty()) {
        projection_ = glm::perspective(glm::radians(camera_.fov()),
                                       engine.aspect_ratio(), 0.1f, 500.0f);

        shader_->set_projection(projection_);
        camera_.set_fov_clean();
    }

    glCheckError();

    glActiveTexture(GL_TEXTURE0);
    texture_->bind();

    // render normal frame
    main_fb_->bind();
    std::array<GLenum, 2> draw_buffers{GL_COLOR_ATTACHMENT0,
                                       GL_COLOR_ATTACHMENT1};
    glDrawBuffers(draw_buffers.size(), draw_buffers.data());
    glCheckError();

    glClearBufferfv(GL_COLOR, 0, clear_color_.data());
    static const std::array<float, 4> black = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 1, black.data());
    glCheckError();

    glClear(GL_DEPTH_BUFFER_BIT);
    md2_->draw(*shader_);

    glCheckError();

    if (glow_) {
        // blur solid image
        blur_fb_->bind();
        blur_shader_->use();
        blur_shader_->set_uniform(disable_blur_loc_, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(1));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screen_quad_->draw(*glow_shader_);

        FrameBuffer::bind_default();
        glow_shader_->use();
        glClear(GL_COLOR_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(0));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(1));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, blur_fb_->color_buffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screen_quad_->draw(*glow_shader_);
    } else {
        FrameBuffer::bind_default();
        blur_shader_->use();
        blur_shader_->set_uniform(disable_blur_loc_, 1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(0));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screen_quad_->draw(*blur_shader_);
    }

    glCheckError();
    draw_ui(engine);
    glCheckError();
}

void MD2View::draw_ui(Engine& engine) {
    static float const vec4width = 275;
    // draw gui
    ImGui::Begin("MD2View");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    if (ImGui::ColorEdit3("Clear color", clear_color_.data())) {
        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], 1.0f);
    }

    if (ImGui::Checkbox("V-sync", &vsync_enabled_)) {
        set_vsync();
    }

    if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {

        camera_.draw_ui();

        if (ImGui::Button("Reset Camera")) {
            reset_camera();
        }

        ImGui::Text("View");
        ImGui::PushItemWidth(vec4width);
        ImGui::InputFloat4("", glm::value_ptr(view_[0]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(view_[1]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(view_[2]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(view_[3]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        ImGui::Text("Projection");
        ImGui::PushItemWidth(vec4width);
        ImGui::InputFloat4("", glm::value_ptr(projection_[0]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(projection_[1]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(projection_[2]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(projection_[3]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("Model", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Model: %s",
                           model_selector_->model_path().c_str());
        int index = md2_->animation_index();

        ImGui::Combo(
            "Animation", &index,
            [](void* data, int idx, char const** out_text) -> bool {
                MD2 const* model = reinterpret_cast<MD2 const*>(data);
                assert(model);
                if (idx < 0 ||
                    static_cast<size_t>(idx) >= model->animations().size()) {
                    return false;
                }
                *out_text = model->animations()[idx].name.c_str();
                return true;
            },
            reinterpret_cast<void*>(md2_.get()), md2_->animations().size());

        md2_->set_animation(static_cast<size_t>(index));

        int sindex = md2_->skin_index();

        ImGui::Combo(
            "Skin", &sindex,
            [](void* data, int idx, char const** out_text) -> bool {
                MD2 const* model = reinterpret_cast<MD2 const*>(data);
                assert(model);
                if (idx < 0 ||
                    static_cast<size_t>(idx) >= model->skins().size()) {
                    return false;
                }
                *out_text = model->skins()[idx].name.c_str();
                return true;
            },
            reinterpret_cast<void*>(md2_.get()), md2_->skins().size());

        md2_->set_skin_index(static_cast<size_t>(sindex));
        load_current_texture(engine); // skin may have changed

        float fps = md2_->frames_per_second();
        ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, "%.3f");
        md2_->set_frames_per_second(fps);

        ImGui::Text("Model");
        ImGui::PushItemWidth(vec4width);
        ImGui::InputFloat4("", glm::value_ptr(model_[0]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(model_[1]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(model_[2]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat4("", glm::value_ptr(model_[3]), "%.3f",
                           ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        ImGui::Checkbox("Glow", &glow_);
        if (ImGui::ColorEdit3("Glow color", glm::value_ptr(glow_color_))) {
            shader_->use();
            shader_->set_uniform(glow_loc_, glow_color_);
        }

        bool model_changed = ImGui::SliderInt("Scale Factor", &scale_, 1, 256);
        model_changed |=
            ImGui::SliderFloat("X-Position", &pos_[0], -7.0f, 7.0f);
        model_changed |=
            ImGui::SliderFloat("Y-Position", &pos_[1], -7.0f, 7.0f);
        model_changed |=
            ImGui::SliderFloat("Z-Position", &pos_[2], -7.0f, 7.0f);
        model_changed |= ImGui::SliderAngle("X-Rotation", &rot_[0]);
        model_changed |= ImGui::SliderAngle("Y-Rotation", &rot_[1]);
        model_changed |= ImGui::SliderAngle("Z-Rotation", &rot_[2]);

        if (ImGui::Button("Reset Model")) {
            reset_model_matrix();
            model_changed = true;
        }
        if (model_changed) {
            update_model();
        }
        ImGui::Image(std::uintptr_t(texture_->id()),
                     ImVec2(texture_->width(), texture_->height()),
                     ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255),
                     ImColor(255, 255, 255, 128));

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

void MD2View::set_vsync() { glfwSwapInterval(vsync_enabled_ ? 1 : 0); }

void MD2View::update(Engine& /* engine */, GLfloat delta_time) {
    md2_->update(delta_time);
}

void MD2View::process_input(Engine& engine, GLfloat delta_time) {
    if (engine.keys()[GLFW_KEY_W]) {
        camera_.move(Camera::FORWARD, delta_time);
    }
    if (engine.keys()[GLFW_KEY_S]) {
        camera_.move(Camera::BACKWARD, delta_time);
    }
    if (engine.keys()[GLFW_KEY_A]) {
        camera_.move(Camera::LEFT, delta_time);
    }
    if (engine.keys()[GLFW_KEY_D]) {
        camera_.move(Camera::RIGHT, delta_time);
    }
}

void MD2View::on_mouse_movement(GLfloat xoffset, GLfloat yoffset) {
    camera_.on_mouse_movement(xoffset, yoffset);
}

void MD2View::on_mouse_scroll(double xoffset, double yoffset) {
    camera_.on_mouse_scroll(xoffset, yoffset);
}
