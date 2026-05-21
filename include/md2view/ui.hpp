#pragma once

class Camera;
class MD2;

/// Backend-agnostic ImGui panels for domain objects.
///
/// These functions depend only on the ImGui widget API and the public
/// interfaces of their arguments — they are not tied to any graphics backend
/// and can be used from both the OpenGL and Vulkan renderers.
namespace UI {

/// Draw an ImGui panel exposing camera position, orientation, and field of
/// view.
void draw(Camera& camera);

/// Draw an ImGui panel for selecting animations, skins, and playback speed.
///
/// @return True if the active skin changed (caller should reload the texture).
[[nodiscard]] bool draw(MD2& md2);

} // namespace UI
