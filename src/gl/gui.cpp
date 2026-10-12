#include "md2view/gl/gui.hpp"
#include "md2view/engine.hpp"

#include <GLFW/glfw3.h>
#include <gsl-lite/gsl-lite.hpp>
#include <imgui.h>
#include <imgui_impl_opengl3.h>

#include <stdexcept>
#include <utility>

namespace GL {

namespace {
// NB: same gl version GL::Renderer uses
constexpr char const* kGlslVersion = "#version 410";
} // namespace

Gui::Gui(::Engine& engine, gsl_lite::not_null<GLFWwindow*> window)
    : engine_(engine)
    , window_(std::move(window)) {
    ImGui::CreateContext();
    if (!ImGui_ImplOpenGL3_Init(kGlslVersion)) {
        ImGui::DestroyContext();
        throw std::runtime_error(
            "failed to initialize the ImGui OpenGL3 backend");
    }
}

// The backend frees its own GL objects, including the font texture.
Gui::~Gui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
}

void Gui::onMouseButton(int button, int action) {
    if (action == GLFW_PRESS && button >= 0 &&
        std::cmp_less(button, mouse_pressed_.size())) {
        gsl_lite::at(mouse_pressed_, button) = true;
    }
}

void Gui::update(double current_time, bool apply_inputs) {
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w{};
    int h{};
    int display_w{};
    int display_h{};
    glfwGetWindowSize(window_, &w, &h);
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    auto const wf = gsl_lite::narrow_cast<float>(w);
    auto const hf = gsl_lite::narrow_cast<float>(h);
    auto const dwf = gsl_lite::narrow_cast<float>(display_w);
    auto const dhf = gsl_lite::narrow_cast<float>(display_h);
    io.DisplaySize = ImVec2(wf, hf);
    io.DisplayFramebufferScale =
        ImVec2(w > 0 ? dwf / wf : 0.0F, h > 0 ? dhf / hf : 0.0F);

    // Setup time step
    io.DeltaTime = time_ > 0.0
                       ? gsl_lite::narrow_cast<float>(current_time - time_)
                       : (1.0f / 60.0f);
    time_ = current_time;

    if (apply_inputs) {
        // Setup inputs
        // (we already got mouse wheel, keyboard keys & characters from glfw
        // callbacks polled in glfwPollEvents())
        if (glfwGetWindowAttrib(window_, GLFW_FOCUSED) > 0) {
            io.MousePos = ImVec2(
                static_cast<float>(engine_.mouse().xpos.value_or(-1)),
                static_cast<float>(engine_.mouse().ypos.value_or(
                    -1))); // Mouse position in screen coordinates (set to -1,-1
                           // if no mouse / on another screen, etc.)
        } else {
            io.MousePos = ImVec2(-1, -1);
        }

        for (int i = 0; i < 3; i++) {
            gsl_lite::at(io.MouseDown, i) = gsl_lite::at(mouse_pressed_, i) ||
                                            glfwGetMouseButton(window_, i) != 0;
            // If a mouse press event came, always pass it as "mouse held this
            // frame", so we don't miss click-release events that are shorter
            // than 1 frame.
            gsl_lite::at(mouse_pressed_, i) = false;
        }

        io.MouseWheel =
            gsl_lite::narrow_cast<float>(engine_.mouse().scrollYOffset);
    }

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(window_, GLFW_CURSOR,
                     io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN
                                        : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
}

void Gui::render() { // NOLINT(readability-convert-member-functions-to-static)
    // NB: this instance owns the imgui context used for rendering here
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace GL
