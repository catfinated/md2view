#include "md2view/gl/gui.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>
#include <utility>

namespace GL {

namespace {
// NB: same gl version GL::Renderer uses
constexpr char const* kGlslVersion = "#version 410";
} // namespace

Gui::Gui(gsl_lite::not_null<GLFWwindow*> window)
    : window_(std::move(window)) {
    ImGui::CreateContext();
    if (!ImGui_ImplGlfw_InitForOpenGL(window_, /*install_callbacks=*/false)) {
        ImGui::DestroyContext();
        throw std::runtime_error("failed to initialize the ImGui GLFW backend");
    }
    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error(
            "failed to initialize the ImGui OpenGL3 backend");
    }
}

// The backend frees its own GL objects, including the font texture.
Gui::~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Gui::update() { // NOLINT(readability-convert-member-functions-to-static)
    // NB: this instance owns the imgui context and backends used here
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::render() { // NOLINT(readability-convert-member-functions-to-static)
    // NB: this instance owns the imgui context used for rendering here
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Gui::onInputFocus(bool focused) { ImGui::GetIO().AddFocusEvent(focused); }

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool Gui::wantsTextInput() const { return ImGui::GetIO().WantTextInput; }

void Gui::onKey(int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window_, key, scancode, action, mods);
}

void Gui::onMouseButton(int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window_, button, action, mods);
}

void Gui::onCursorPos(double xpos, double ypos) {
    ImGui_ImplGlfw_CursorPosCallback(window_, xpos, ypos);
}

void Gui::onScroll(double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window_, xoffset, yoffset);
}

void Gui::onChar(unsigned int codepoint) {
    ImGui_ImplGlfw_CharCallback(window_, codepoint);
}

} // namespace GL
