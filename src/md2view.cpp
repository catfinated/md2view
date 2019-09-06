#include "engine.hpp"
#include "camera.hpp"
#include "md2.hpp"
#include "model_selector.hpp"

#include "glm/gtx/string_cast.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>

class MD2View
{
public:
    MD2View();

    bool on_engine_initialized(EngineBase& engine);
    void process_input(EngineBase& engine, GLfloat delta_time);
    void on_mouse_movement(GLfloat xoffset, GLfloat yoffset);
    void on_framebuffer_resized(float);
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
    std::shared_ptr<Shader> shader_;
    Camera camera_;
    boost::program_options::options_description options_;
    std::string models_dir_;
    ModelSelector ms_;
    bool vsync_enabled_ = false;
    float scale_ = 64.0f;
    std::array<float, 3> rot_;
    glm::vec3 pos_;
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
    rot_[1] = glm::radians(-90.0f); // quake used different world matrix
    rot_[2] = 0.0f;
    scale_ = 64.0f;

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

    shader_ = engine.resource_manager().load_shader("md2.vert", "md2.frag", nullptr, "md2");
    std::cout << ms_.model_name() << '\n';
    std::cout << ms_.model().skins().size() << '\n';
    load_current_texture(engine);

    camera_.set_position(glm::vec3(0.0f, 0.0f, 3.0f));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    std::cout << "done on engine init\n";
    glCheckError();
    return true;
}

void MD2View::on_framebuffer_resized(float aspect_ratio)
{
    glm::mat4 projection = glm::perspective(glm::radians(camera_.zoom()),
                                           aspect_ratio,
                                           0.1f, 500.0f);

    shader_->set_projection(projection);
}

void MD2View::render(EngineBase& engine)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //ImGui::ShowTestWindow(nullptr);
    //return;

    auto& texture = engine.resource_manager().texture2D(ms_.model().current_skin().fpath);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 view = camera_.view_matrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera_.zoom()),
                                            engine.aspect_ratio(),
                                            0.1f, 500.0f);

    //translate, rotate, scale
    glm::mat4 model(1.0);
    model = glm::translate(model, pos_);
    glm::quat rotx = glm::angleAxis(rot_[0], glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat roty = glm::angleAxis(rot_[1], glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat rotz = glm::angleAxis(rot_[2], glm::vec3(0.0f, 0.0f, 1.0f));
    model *= glm::mat4_cast(roty * rotz * rotx);
    auto s = 1.0f / scale_; // uniform scale factor
    model = glm::scale(model, glm::vec3(s, s, s));

    shader_->use();
    shader_->set_model(model);
    shader_->set_view(view);
    shader_->set_projection(projection);

    texture.bind();
    ms_.model().draw(*shader_);
    glCheckError();

    // draw gui
    ImGui::Begin("Scene");

    //ImGui::ColorEdit3("clear color", (float*)&clear_color);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    bool vsync = vsync_enabled_;
    ImGui::Checkbox("V-sync", &vsync);

    if (vsync != vsync_enabled_) {
        vsync_enabled_ = vsync;
        glfwSwapInterval(vsync ? 1 : 0);
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

    ImGui::End();

    ImGui::Begin("Model");

    if (ImGui::Button("Random Model")) {
        ms_.select_random_model(engine.random_engine());
    }

    ms_.draw_ui();

    load_current_texture(engine);

    ImGui::Text("Model");
    ImGui::PushItemWidth(vec4width);
    ImGui::InputFloat4("", glm::value_ptr(model[0]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[1]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[2]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[3]), precision, ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::InputFloat("Scale Factor", &scale_, 1.0f, 5.0f, 1);
    scale_ = std::max(1.0f, std::min(scale_, 256.0f));
    ImGui::SliderFloat("X-Position", &pos_[0], -7.0f, 7.0f);
    ImGui::SliderFloat("Y-Position", &pos_[1], -7.0f, 7.0f);
    ImGui::SliderFloat("Z-Position", &pos_[2], -7.0f, 7.0f);
    ImGui::SliderAngle("X-Rotation", &rot_[0]);
    ImGui::SliderAngle("Y-Rotation", &rot_[1]);
    ImGui::SliderAngle("Z-Rotation", &rot_[2]);

    if (ImGui::Button("Reset Model")) {
        reset_model_matrix();
    }

    ImGui::End();

    ImGui::Begin("Texture");

    ImGui::Image(reinterpret_cast<void*>(texture.id()), ImVec2(texture.width(), texture.height()),
                 ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(255,255,255,128));

    ImGui::End();
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
    camera_.process_mouse_movement(xoffset, yoffset);
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
