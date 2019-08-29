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

private:
    //blue::md2::Model model_;
    //MD2Model model_;
    Camera camera_;
    std::unique_ptr<TexturedQuad> quad_;
    boost::program_options::options_description options_;
    std::string model_name_;
    std::string anim_name_;
    ModelSelector ms_;
};

MD2View::MD2View()
    : options_("Game options")
    , ms_("../src/models")
{
    options_.add_options()
        ("model,m", boost::program_options::value<std::string>(&model_name_)/*->required()*/, "MD2 Model")
        ("anim,a", boost::program_options::value<std::string>(&anim_name_), "Model Animation");
}

bool MD2View::parse_args(EngineBase& engine)
{
    if (model_name_.empty()) {
        std::cerr << "no MD2 model name specified\n";
        return false;
    }

    return true;
}

void MD2View::reset_camera()
{
    camera_.reset();
    camera_.Position = glm::vec3(0.0f, 0.0f, 3.0f);
}

bool MD2View::on_engine_initialized(EngineBase& engine)
{
    ms_.init();

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    //boost::filesystem::path p(engine.resource_manager().models_dir());
    //p /= model_name_;
    //p /= "tris.md2";
    //BLUE_EXPECT(model_.load(p.string()));

    //if (!anim_name_.empty()) {
    //    model_.set_animation(anim_name_);
    // }
    //else {
    //    model_.set_animation(0);
    // }

    engine.resource_manager().load_shader("md2.vert", "md2.frag", nullptr, "md2");
    engine.resource_manager().load_shader("md2_model.vert", "md2_model.frag", nullptr, "md2_model");
    engine.resource_manager().load_shader("blending.vert", "blending.frag", nullptr, "quad");
    //engine.resource_manager().load_texture2D(model_.skins()[0].c_str(), "skin", false);
    std::cout << ms_.model_name() << '\n';
    std::cout << ms_.model().skins().size() << '\n';
    engine.resource_manager().load_texture2D(ms_.model().skins()[0].c_str(), ms_.model_name(), false);

    camera_.Position = glm::vec3(0.0f, 0.0f, 3.0f);

    //quad_.reset(new TexturedQuad(model_.skins()[0]));

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LEQUAL);

    std::cout << "done on engine init\n";
    glCheckError();
    return true;
}

void MD2View::render(EngineBase& engine)
{
    auto& shader = engine.resource_manager().shader("md2");
    //auto& shader = engine.resource_manager().shader("quad");
    //auto& texture = engine.resource_manager().texture2D("skin");
    auto& texture = engine.resource_manager().texture2D(ms_.model_name());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glm::mat4 view = camera_.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera_.Zoom),
                                            engine.aspect_ratio(),
                                            0.1f, 500.0f);

    //translate, rotate, scale
    glm::mat4 model(1.0);
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(1.0f/64.0f, 1.0f/64.0f, 1.0f/64.0f));
    //std::cout << glm::to_string(model) << '\n';

    shader.use();
    shader.set_model(model);
    shader.set_view(view);
    shader.set_projection(projection);

    texture.bind();
    ms_.model().draw(shader);
    glCheckError();
    //quad_->Draw(shader);

    // draw gui
    ImGui::Text("Camera");

    ImGui::InputFloat3("Position", glm::value_ptr(camera_.Position), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Front", glm::value_ptr(camera_.Front), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Up", glm::value_ptr(camera_.Up), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("Right", glm::value_ptr(camera_.Right), -1, ImGuiInputTextFlags_ReadOnly);

    if (ImGui::Button("Reset Camera")) {
        reset_camera();
    }

    int model_index = ms_.model_index();

    ImGui::ListBox("Model", &model_index,
                   [](void * data, int idx, char const ** out) -> bool {
                       auto ms = reinterpret_cast<ModelSelector const *>(data);
                       if (idx < 0 || idx >= ms->items().size()) { return false; }
                       *out = ms->items()[idx].c_str();
                       return true;
                   },
                   reinterpret_cast<void *>(&ms_),
                   ms_.items().size());

    ms_.set_model_index(model_index);
    engine.resource_manager().load_texture2D(ms_.model().skins()[0].c_str(), ms_.model_name(), false);


    int index = ms_.model().animation_index();

    ImGui::Combo("Animation", &index,
                 [](void * data, int idx, char const ** out_text) -> bool {
                     blue::md2::Model const * model = reinterpret_cast<blue::md2::Model const *>(data);
                     assert(model);
                     if (idx < 0 || idx >= model->animations().size()) { return false; }
                     *out_text = model->animations()[idx].name.c_str();
                     return true;
                 },
                 reinterpret_cast<void *>(&ms_.model()),
                 ms_.model().animations().size());

    ms_.model().set_animation(static_cast<size_t>(index));

    float fps = ms_.model().frames_per_second();
    //ImGui::SliderFloat("Animation FPS", &fps, 1.0f, 60.0f);
    ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, 1);
    ms_.model().set_frames_per_second(fps);

    //ImGui::ColorEdit3("clear color", (float*)&clear_color);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Begin("Model");

    ImGui::Text("View");
    ImGui::InputFloat4("", glm::value_ptr(view[0]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[1]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[2]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(view[3]), -1, ImGuiInputTextFlags_ReadOnly);

    ImGui::Text("Projection");
    ImGui::InputFloat4("", glm::value_ptr(projection[0]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[1]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[2]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(projection[3]), -1, ImGuiInputTextFlags_ReadOnly);

    ImGui::Text("Model");
    ImGui::InputFloat4("", glm::value_ptr(model[0]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[1]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[2]), -1, ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat4("", glm::value_ptr(model[3]), -1, ImGuiInputTextFlags_ReadOnly);

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
