#define STB_IMAGE_IMPLEMENTATION

#include "engine.hpp"
#include "camera.hpp"
#include "md2.hpp"
#include "model_selector.hpp"
#include "frame_buffer.hpp"
#include "screen_quad.hpp"

#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <memory>

class MD2View
{
public:
    MD2View();

    bool on_engine_initialized(EngineBase& engine);
    void process_input(EngineBase& engine, GLfloat delta_time);
    void on_mouse_movement(GLfloat xoffset, GLfloat yoffset);
    void on_mouse_scroll(double xoffset, double yoffset);
    void on_framebuffer_resized(int, int);
    void update(EngineBase& engine, GLfloat delta_time);
    void render(EngineBase& engine);
    char const * const title() const { return "MD2View"; }

    boost::program_options::options_description& options() { return options_; }
    bool parse_args(EngineBase& engine);

private:
    void reset_camera();
    void reset_model_matrix();
    void load_current_texture(EngineBase&);
    void update_model();

private:
    std::shared_ptr<Shader> shader_;
    std::shared_ptr<Shader> blur_shader_;
    std::shared_ptr<Shader> glow_shader_;
    Camera camera_;
    boost::program_options::options_description options_;
    std::string models_dir_;
    ModelSelector ms_;
    bool vsync_enabled_ = false;
    int scale_ = 64.0f;
    std::array<float, 3> rot_;
    glm::vec3 pos_;
    glm::mat4 model_;
    glm::mat4 view_;
    glm::mat4 projection_;
    std::array<float, 4> clear_color_;
    std::unique_ptr<FrameBuffer<1, false>> blur_fb_;
    std::unique_ptr<FrameBuffer<2, true>> main_fb_;
    ScreenQuad screen_quad_;
    GLint disable_blur_loc_;
    Texture2D * texture_ = nullptr;
    bool glow_ = false;
    glm::vec3 glow_color_;
    GLint glow_loc_;
};

MD2View::MD2View()
    : options_("Game options")
{
    reset_model_matrix();

    options_.add_options()
        ("models,m",
         boost::program_options::value<std::string>(&models_dir_),
         "MD2 Models Directory or PAK file");
}

bool MD2View::parse_args(EngineBase& engine)
{
    if (models_dir_.empty()) {
        models_dir_ = "../data/models";
    }

    return true;
}

void MD2View::reset_model_matrix()
{
    rot_[0] = 0.0f;
    rot_[1] = glm::radians(-90.0f); // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64;

    pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
}

void MD2View::reset_camera()
{
    camera_.reset(glm::vec3(0.0f, 0.0f, 3.0f));
}

void MD2View::load_current_texture(EngineBase& engine)
{
    auto const& path = ms_.model().current_skin().fpath;

    if (ms_.pak()) {
        texture_ = std::addressof(engine.resource_manager().load_texture2D(*ms_.pak(), path));
    }
    else {
        texture_ = std::addressof(engine.resource_manager().load_texture2D(path));
    }
}

bool MD2View::on_engine_initialized(EngineBase& engine)
{
    // init objects which needed an opengl context to initialize
    ms_.init(models_dir_, engine);
    blur_fb_.reset(new FrameBuffer<1, false>(engine.width(), engine.height()));
    main_fb_.reset(new FrameBuffer<2, true>(engine.width(), engine.height()));
    screen_quad_.init();

    clear_color_ = { 0.2f, 0.2f, 0.2f, 1.0f };
    glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], 1.0f);

    shader_ = engine.resource_manager().load_shader("md2.vert", "md2.frag", nullptr, "md2");
    shader_->use();
    update_model();
    load_current_texture(engine);
    glow_loc_ = shader_->uniform_location("glow_color");
    glow_color_ = glm::vec3(0.0f, 1.0f, 0.0f);
    shader_->set_uniform(glow_loc_, glow_color_);

    blur_shader_ = engine.resource_manager().load_shader("screen.vert", "blur.frag", nullptr, "blur");
    blur_shader_->use();
    disable_blur_loc_ = blur_shader_->uniform_location("disable_blur");
    blur_shader_->set_uniform(disable_blur_loc_, 1);

    glow_shader_ = engine.resource_manager().load_shader("screen.vert", "glow.frag", nullptr, "glow");
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
    //glDisable(GL_BLEND);
    //glEnable(GL_BLEND);
    FrameBuffer<>::bind_default();

    spdlog::info("done on engine init");
    glCheckError();
    return true;
}

void MD2View::on_framebuffer_resized(int width, int height)
{
    glm::mat4 projection = glm::perspective(glm::radians(camera_.fov()),
                                            static_cast<float>(width) / static_cast<float>(height),
                                            0.1f, 500.0f);

    shader_->use();
    shader_->set_projection(projection);

    FrameBuffer<>::bind_default();
    main_fb_.reset(new FrameBuffer<2, true>(width, height));
    main_fb_->bind();
    glViewport(0, 0, width, height);

    blur_fb_.reset(new FrameBuffer<1, false>(width, height));
    blur_fb_->bind();
    glViewport(0, 0, width, height);
    FrameBuffer<>::bind_default();

    glViewport(0, 0, width, height);
}

void MD2View::update_model()
{
    //translate, rotate, scale
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

void MD2View::render(EngineBase& engine)
{
    //ImGui::ShowTestWindow(nullptr);
    //return;
    shader_->use();

    if (camera_.view_dirty()) {
        view_ = camera_.view_matrix();
        shader_->set_view(view_);
        camera_.set_view_clean();
    }

    if (camera_.fov_dirty()) {
        projection_ = glm::perspective(glm::radians(camera_.fov()),
                                                    engine.aspect_ratio(),
                                                    0.1f, 500.0f);

        shader_->set_projection(projection_);
        camera_.set_fov_clean();
    }

    glCheckError();

    glActiveTexture(GL_TEXTURE0);
    texture_->bind();

    // render normal frame
    main_fb_->bind();  
    GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, draw_buffers);
    glCheckError();

    glClearBufferfv(GL_COLOR, 0, clear_color_.data());
    static const std::array<float, 4> black = { 0.0f, 0.0f, 0.0f, 0.0f };
    glClearBufferfv(GL_COLOR, 1, black.data());
    glCheckError();

    glClear(GL_DEPTH_BUFFER_BIT);
    ms_.model().draw(*shader_);

    glCheckError();

    if (glow_) {
        // blur solid image
        blur_fb_->bind();
        blur_shader_->use();
        blur_shader_->set_uniform(disable_blur_loc_, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(1));
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screen_quad_.draw(*glow_shader_);

        blur_fb_->bind_default();
        glClear(GL_COLOR_BUFFER_BIT);
        glow_shader_->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(0));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(1));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, blur_fb_->color_buffer(0));
        screen_quad_.draw(*glow_shader_);
    } 
    else {
        main_fb_->bind_default();
        blur_shader_->use();
        glActiveTexture(GL_TEXTURE0);;
        glBindTexture(GL_TEXTURE_2D, main_fb_->color_buffer(0));
        blur_shader_->set_uniform(disable_blur_loc_, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        screen_quad_.draw(*blur_shader_);
    }

    glCheckError();

    // draw gui
    ImGui::Begin("Scene");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    if (ImGui::ColorEdit3("Clear color", clear_color_.data())) {
        glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], 1.0f);
    }

    if (ImGui::Checkbox("V-sync", &vsync_enabled_)) {
        glfwSwapInterval(vsync_enabled_ ? 1 : 0);
    }

    ImGui::Text("Camera");
    camera_.draw_ui();

    if (ImGui::Button("Reset Camera")) {
        reset_camera();
    }

    static float const vec4width = 275;
    static int const precision = 5;

    ImGui::Text("View");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(view_[0]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view_[1]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view_[2]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view_[3]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Text("Projection");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(projection_[0]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection_[1]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection_[2]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection_[3]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::End();

    ImGui::Begin("Model");

    if (ImGui::Button("Random Model")) {
        ms_.select_random_model(engine.random_engine());
    }

    ms_.draw_ui();

    load_current_texture(engine);

    ImGui::Text("Model");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(model_[0]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model_[1]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model_[2]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model_[3]), "%.3f", ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Checkbox("Glow", &glow_);
    if (ImGui::ColorEdit3("Glow color", glm::value_ptr(glow_color_))) {
        shader_->use();
        shader_->set_uniform(glow_loc_, glow_color_);
    }

    bool model_changed = ImGui::SliderInt("Scale Factor", &scale_, 1, 256);
    model_changed |= ImGui::SliderFloat("X-Position", &pos_[0], -7.0f, 7.0f);
    model_changed |= ImGui::SliderFloat("Y-Position", &pos_[1], -7.0f, 7.0f);
    model_changed |= ImGui::SliderFloat("Z-Position", &pos_[2], -7.0f, 7.0f);
    model_changed |= ImGui::SliderAngle("X-Rotation", &rot_[0]);
    model_changed |= ImGui::SliderAngle("Y-Rotation", &rot_[1]);
    model_changed |= ImGui::SliderAngle("Z-Rotation", &rot_[2]);

    if (ImGui::Button("Reset Model")) {
        reset_model_matrix();
        model_changed = true;
    }

    ImGui::End();

    ImGui::Begin("Texture");

    ImGui::Image(reinterpret_cast<void*>(std::uintptr_t(texture_->id())), ImVec2(texture_->width(), texture_->height()),
                 ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(255,255,255,128));

    ImGui::End();

    if (model_changed) { update_model(); }
}

void MD2View::update(EngineBase& engine, GLfloat delta_time)
{
    ms_.model().update(delta_time);
}

void MD2View::process_input(EngineBase& engine, GLfloat delta_time)
{
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

void MD2View::on_mouse_movement(GLfloat xoffset, GLfloat yoffset)
{
    camera_.on_mouse_movement(xoffset, yoffset);
}

void MD2View::on_mouse_scroll(double xoffset, double yoffset)
{
    camera_.on_mouse_scroll(xoffset, yoffset);
}

int main(int argc, char const * argv[])
{
    Engine<MD2View> engine;

    if (!engine.init(argc, argv)) {
        return EXIT_FAILURE;
    }

    engine.run_game();

    return EXIT_SUCCESS;
}
