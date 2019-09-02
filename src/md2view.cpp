#include "engine.hpp"
#include "camera.hpp"
#include "md2.hpp"
#include "quad.hpp"
#include "model_selector.hpp"

#include "glm/gtx/string_cast.hpp"

#include <algorithm>
#include <memory>

using namespace blue;

class MD2View
{
public:
    MD2View();

    bool on_engine_initialized(EngineBase& engine);
    void process_input(EngineBase& engine, GLfloat delta_time);
    void on_mouse_movement(GLfloat xoffset, GLfloat yoffset);
    void update(EngineBase& engine, GLfloat delta_time);
    void render(EngineBase& engine);
    char const * const title() const { return "MD2View"; }

    boost::program_options::options_description& options() { return options_; }
    bool parse_args(EngineBase& engine);

private:
    void reset_camera();
    void reset_model_matrix();
    void load_current_texture(EngineBase&);

private:
    Camera camera_;
    std::unique_ptr<TexturedQuad> quad_;
    boost::program_options::options_description options_;
    std::string models_dir_;
    std::string anim_name_;
    ModelSelector ms_;
    bool vsync_enabled_ = false;
    float scale_ = 64.0f;
    std::array<float, 3> rot_;
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
        models_dir_ = "../src/models";
    }

    return true;
}

void MD2View::reset_model_matrix()
{
    rot_[0] = 0.0f;
    rot_[1] = -90.0f; // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64.0f;

}

void MD2View::reset_camera()
{
    camera_.reset();
    camera_.Position = glm::vec3(0.0f, 0.0f, 3.0f);
}

void MD2View::load_current_texture(EngineBase& engine)
{
    auto const& path = ms_.model().current_skin().fpath;

    if (ms_.pak()) {
        engine.resource_manager().load_texture2D(*ms_.pak(), path);
    }
    else {
        engine.resource_manager().load_texture2D(path);
    }
}

bool MD2View::on_engine_initialized(EngineBase& engine)
{
    ms_.init(models_dir_, engine);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    engine.resource_manager().load_shader("md2.vert", "md2.frag", nullptr, "md2");
    //engine.resource_manager().load_shader("md2_model.vert", "md2_model.frag", nullptr, "md2_model");
    engine.resource_manager().load_shader("blending.vert", "blending.frag", nullptr, "quad");
    std::cout << ms_.model_name() << '\n';
    std::cout << ms_.model().skins().size() << '\n';
    load_current_texture(engine);

    camera_.Position = glm::vec3(0.0f, 0.0f, 3.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    std::cout << "done on engine init\n";
    glCheckError();
    return true;
}

void MD2View::render(EngineBase& engine)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //ImGui::ShowTestWindow(nullptr);
    //return;
    //std::cout << " model path=" << ms_.model_path() << '\n';

    auto& shader = engine.resource_manager().shader("md2");
    auto& texture = engine.resource_manager().texture2D(ms_.model().current_skin().fpath);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 view = camera_.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera_.Zoom),
                                            engine.aspect_ratio(),
                                            0.1f, 500.0f);

    //translate, rotate, scale
    glm::mat4 model(1.0);
    glm::quat rotx = glm::angleAxis(glm::radians(rot_[0]), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat roty = glm::angleAxis(glm::radians(rot_[1]), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat rotz = glm::angleAxis(glm::radians(rot_[2]), glm::vec3(0.0f, 0.0f, 1.0f));
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::mat4_cast(roty * rotz * rotx);
    auto s = 1.0f / scale_; // uniform scale factor
    model = glm::scale(model, glm::vec3(s, s, s));

    shader.use();
    shader.set_model(model);
    shader.set_view(view);
    shader.set_projection(projection);

    texture.bind();
    ms_.model().draw(shader);
    glCheckError();
    //quad_->Draw(shader);

    // draw gui
    ImGui::Begin("Model");
    ImGui::Text("Camera");

    ImGui::InputFloat3("Position", glm::value_ptr(camera_.Position), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Front", glm::value_ptr(camera_.Front), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", glm::value_ptr(camera_.Up), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", glm::value_ptr(camera_.Right), -1, ImGuiInputTextFlags_ReadOnly);

    if (ImGui::Button("Reset Camera")) {
        reset_camera();
    }

    bool vsync = vsync_enabled_;
    ImGui::Checkbox("V-sync", &vsync);

    if (vsync != vsync_enabled_) {
        vsync_enabled_ = vsync;
        glfwSwapInterval(vsync ? 1 : 0);
    }

    if (ImGui::Button("Random Model")) {
        ms_.select_random_model(engine.random_engine());
    }

    ms_.draw_ui();

    //ImGui::ColorEdit3("clear color", (float*)&clear_color);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    load_current_texture(engine);

    ImGui::Begin("Matrices");

    static float const vec4width = 275;
    static int const precision = 5;

    ImGui::Text("View");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(view[0]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[1]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[2]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[3]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Text("Projection");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(projection[0]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[1]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[2]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[3]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Text("Model");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(model[0]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[1]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[2]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[3]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::InputFloat("Scale Factor", &scale_, 1.0f, 5.0f, 1);
    ImGui::SliderFloat("X-Rotation", &rot_[0], -360.0f, 360.0f);
    ImGui::SliderFloat("Y-Rotation", &rot_[1], -360.0f, 360.0f);
    ImGui::SliderFloat("Z-Rotation", &rot_[2], -360.0f, 360.0f);

    if (ImGui::Button("Reset Model")) {
        reset_model_matrix();
    }

    ImGui::End();
}

void MD2View::update(EngineBase& engine, GLfloat delta_time)
{
    ms_.model().update(delta_time);
}

void MD2View::process_input(EngineBase& engine, GLfloat delta_time)
{
    if (engine.keys()[GLFW_KEY_W]) {
        camera_.ProcessKeyboard(FORWARD, delta_time);
    }
    if (engine.keys()[GLFW_KEY_S]) {
        camera_.ProcessKeyboard(BACKWARD, delta_time);
    }
    if (engine.keys()[GLFW_KEY_A]) {
        camera_.ProcessKeyboard(LEFT, delta_time);
    }
    if (engine.keys()[GLFW_KEY_D]) {
        camera_.ProcessKeyboard(RIGHT, delta_time);
    }
}

void MD2View::on_mouse_movement(GLfloat xoffset, GLfloat yoffset)
{
    camera_.ProcessMouseMovement(xoffset, yoffset);
}

int main(int argc, char const * argv[])
{
    GLFWEngine<MD2View> game;
    if (game.init(argc, argv)) {
        game.run();
    }
}
