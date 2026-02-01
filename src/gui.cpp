#include "md2view/gui.hpp"
#include "md2view/engine.hpp"

#include <gsl-lite/gsl-lite.hpp>

#include <array>
#include <utility>

Gui::Gui(Engine& engine, gsl_lite::not_null<GLFWwindow*> window)
    : engine_(engine)
    , window_(std::move(window)) {
    init();
}

void Gui::init() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // io.RenderDrawListsFn = nullptr; // why no user data callback?!?

    // Backup GL state
    GLint last_texture{};
    GLint last_array_buffer{};
    GLint last_vertex_array{};
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    glCheckError();

    auto shader = engine_.resource_manager().load_shader("imgui");

    attrib_location_tex_ = shader->uniform_location("texture0");
    attrib_location_projection_ = shader->uniform_location("projection");
    attrib_location_position_ = 0; // shader.uniform_location("position");
    attrib_location_uv_ = 1;       // shader.uniform_location("uv");
    attrib_location_color_ = 2;    // shader.uniform_location("color");

    spdlog::info("texture: {}", attrib_location_tex_);
    spdlog::info("projection: {}", attrib_location_projection_);
    spdlog::info("position: {}", attrib_location_position_);
    spdlog::info("uv: {}", attrib_location_uv_);
    spdlog::info("color: {}", attrib_location_color_);

    glCheckError();

    glGenBuffers(static_cast<GLsizei>(Buffer::num_buffers), glbuffers_.data());
    glGenVertexArrays(1, std::addressof(vao_));

    glCheckError();

    glBindVertexArray(vao_);
    glCheckError();
    glBindBuffer(GL_ARRAY_BUFFER,
                 glbuffers_[static_cast<size_t>(Buffer::vertex)]);
    glCheckError();
    glEnableVertexAttribArray(attrib_location_position_);
    glCheckError();
    glEnableVertexAttribArray(attrib_location_uv_);
    glCheckError();
    glEnableVertexAttribArray(attrib_location_color_);
    glCheckError();

    glCheckError();

    glVertexAttribPointer(attrib_location_position_, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ImDrawVert),
                          GL_BUFFER_OFFSET(offsetof(ImDrawVert, pos)));
    glCheckError();
    glVertexAttribPointer(attrib_location_uv_, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ImDrawVert),
                          GL_BUFFER_OFFSET(offsetof(ImDrawVert, uv)));
    glCheckError();
    glVertexAttribPointer(attrib_location_color_, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(ImDrawVert),
                          GL_BUFFER_OFFSET(offsetof(ImDrawVert, col)));
    glCheckError();

    // create font
    // Build texture atlas
    unsigned char* pixels{nullptr};
    int width{};
    int height{};
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // Upload texture to graphics system
    glGenTextures(1, std::addressof(font_texture_));
    glBindTexture(GL_TEXTURE_2D, font_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (intptr_t)font_texture_;

    // Restore state
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindVertexArray(last_vertex_array);

    glCheckError();
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
    io.DeltaTime = time_ > 0.0 ? (float)(current_time - time_) : (1.0f / 60.0f);
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

        io.MouseWheel = gsl_lite::narrow_cast<float>(
            engine_.mouse().scroll_yoffset.value_or(0.0));
    }

    // Hide OS mouse cursor if ImGui is drawing it
    glfwSetInputMode(window_, GLFW_CURSOR,
                     io.MouseDrawCursor ? GLFW_CURSOR_HIDDEN
                                        : GLFW_CURSOR_NORMAL);

    // Start the frame
    ImGui::NewFrame();
}

void Gui::render() {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Render();
    auto* draw_data = ImGui::GetDrawData();

    int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width == 0 || fb_height == 0) {
        return;
    }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLint last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
    glActiveTexture(GL_TEXTURE0);
    GLint last_program;
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    GLint last_texture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_array_buffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    GLint last_element_array_buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
    GLint last_vertex_array;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
    GLint last_blend_src_rgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
    GLint last_blend_dst_rgb;
    glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
    GLint last_blend_src_alpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
    GLint last_blend_dst_alpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
    GLint last_blend_equation_rgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
    GLint last_blend_equation_alpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
    std::array<GLint, 4> last_viewport{};
    glGetIntegerv(GL_VIEWPORT, last_viewport.data());
    std::array<GLint, 4> last_scissor_box{};
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box.data());
    GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
    GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth
    // testing, scissor enabled
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    // Setup viewport, orthographic projection matrix
    glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glm::mat4 const ortho_projection = {
        {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };
    // glUseProgram(g_ShaderHandle);
    auto shader = engine_.resource_manager().shader("imgui");
    shader->use();

    glUniform1i(attrib_location_tex_, 0);
    // glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE,
    // &ortho_projection[0][0]);
    shader->set_projection(ortho_projection);
    glBindVertexArray(vao_);

    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = nullptr;

        glBindBuffer(GL_ARRAY_BUFFER,
                     glbuffers_[static_cast<size_t>(Buffer::vertex)]);
        glBufferData(GL_ARRAY_BUFFER,
                     gsl_lite::narrow_cast<GLsizeiptr>(
                         cmd_list->VtxBuffer.Size * sizeof(ImDrawVert)),
                     (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                     glbuffers_[static_cast<size_t>(Buffer::element)]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     gsl_lite::narrow_cast<GLsizeiptr>(
                         cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx)),
                     (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                auto const yf =
                    gsl_lite::narrow_cast<float>(fb_height) - pcmd->ClipRect.w;

                glScissor(gsl_lite::narrow_cast<int>(pcmd->ClipRect.x),
                          gsl_lite::narrow_cast<int>(yf),
                          gsl_lite::narrow_cast<int>(pcmd->ClipRect.z -
                                                     pcmd->ClipRect.x),
                          gsl_lite::narrow_cast<int>(pcmd->ClipRect.w -
                                                     pcmd->ClipRect.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                                      : GL_UNSIGNED_INT,
                               idx_buffer_offset);
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glUseProgram(last_program);
    glBindTexture(GL_TEXTURE_2D, last_texture);
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb,
                        last_blend_src_alpha, last_blend_dst_alpha);

    if (last_enable_blend != 0U) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    if (last_enable_cull_face != 0U) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }

    if (last_enable_depth_test != 0U) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    if (last_enable_scissor_test != 0U) {
        glEnable(GL_SCISSOR_TEST);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }

    glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2],
               (GLsizei)last_viewport[3]);
    glScissor(last_scissor_box[0], last_scissor_box[1],
              (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
}

void Gui::shutdown() {
    // TODO
}
