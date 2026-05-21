#include "md2view/ui.hpp"
#include "md2view/camera.hpp"
#include "md2view/md2.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>

namespace UI {

void draw(Camera& camera) {
    auto position = camera.position();
    ImGui::InputFloat3("Position", glm::value_ptr(position), "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    auto front = camera.front();
    ImGui::InputFloat3("Front", glm::value_ptr(front), "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    auto up = camera.up();
    ImGui::InputFloat3("Up", glm::value_ptr(up), "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    auto right = camera.right();
    ImGui::InputFloat3("Right", glm::value_ptr(right), "%.3f",
                       ImGuiInputTextFlags_ReadOnly);
    float fov = camera.fov();
    if (ImGui::SliderFloat("fov", &fov, 1.0f, 60.0f)) {
        camera.set_fov(fov);
    }
    float pitch = camera.pitch();
    ImGui::InputFloat("Pitch", &pitch, 0.0f, 0.0f, "%.3f",
                      ImGuiInputTextFlags_ReadOnly);
    float yaw = camera.yaw();
    ImGui::InputFloat("Yaw", &yaw, 0.0f, 0.0f, "%.3f",
                      ImGuiInputTextFlags_ReadOnly);
}

bool draw(MD2& md2) {
    int index = gsl_lite::narrow_cast<int>(md2.animation_index());
    constexpr int anim_max_height_in_items = 15;

    ImGui::Combo(
        "Animation", &index,
        [](void* data, int idx) -> char const* {
            auto const* m = static_cast<MD2 const*>(data);
            gsl_Assert(static_cast<size_t>(idx) < m->animations().size());
            return m->animations()[idx].name.c_str();
        },
        &md2, gsl_lite::narrow_cast<int>(md2.animations().size()),
        anim_max_height_in_items);

    md2.set_animation(static_cast<size_t>(index));

    int sindex = gsl_lite::narrow_cast<int>(md2.skin_index());

    ImGui::Combo(
        "Skin", &sindex,
        [](void* data, int idx) -> char const* {
            auto const* m = static_cast<MD2 const*>(data);
            gsl_Assert(static_cast<size_t>(idx) < m->skins().size());
            return m->skins()[idx].name.c_str();
        },
        &md2, gsl_lite::narrow_cast<int>(md2.skins().size()));

    float fps = md2.frames_per_second();
    ImGui::InputFloat("Animation FPS", &fps, 1.0f, 5.0f, "%.3f");
    md2.set_frames_per_second(fps);

    if (static_cast<size_t>(sindex) == md2.skin_index()) {
        return false;
    }

    md2.set_skin_index(static_cast<size_t>(sindex));
    return true;
}

} // namespace UI
