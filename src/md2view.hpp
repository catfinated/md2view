#pragma once

#include "camera.hpp"
#include "md2.hpp"
#include "model_selector.hpp"
#include "frame_buffer.hpp"
#include "screen_quad.hpp"
#include "texture2D.hpp"

#include <glm/glm.hpp>

#include <string>
#include <memory>

class Engine;

class MD2View
{
public:
    MD2View();

    bool on_engine_initialized(Engine& engine);
    void process_input(Engine& engine, GLfloat delta_time);
    void on_mouse_movement(GLfloat xoffset, GLfloat yoffset);
    void on_mouse_scroll(double xoffset, double yoffset);
    void on_framebuffer_resized(int, int);
    void update(Engine& engine, GLfloat delta_time);
    void render(Engine& engine);
    char const * const title() const { return "MD2View"; }

private:
    void reset_camera();
    void reset_model_matrix();
    void load_current_texture(Engine&);
    void update_model();
    void draw_ui(Engine&);
    void set_vsync();
    void load_model(Engine&);

private:
    std::shared_ptr<MD2> md2_;
    std::unique_ptr<ModelSelector> model_selector_;
    std::shared_ptr<Texture2D> texture_;
    std::shared_ptr<Shader> shader_;
    std::shared_ptr<Shader> blur_shader_;
    std::shared_ptr<Shader> glow_shader_;
    std::unique_ptr<ScreenQuad> screen_quad_;
    std::unique_ptr<FrameBuffer> blur_fb_;
    std::unique_ptr<FrameBuffer> main_fb_;

    Camera camera_;
    std::string models_dir_;
    bool vsync_enabled_ = true;
    int scale_ = 64.0f;
    std::array<float, 3> rot_;
    glm::vec3 pos_;
    glm::mat4 model_;
    glm::mat4 view_;
    glm::mat4 projection_;
    std::array<float, 4> clear_color_;
    GLint disable_blur_loc_;
    bool glow_ = false;
    glm::vec3 glow_color_;
    GLint glow_loc_;
};
