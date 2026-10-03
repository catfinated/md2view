# Vulkan Renderer Roadmap

This document tracks the work needed to bring the Vulkan backend (`vkmd2v`) to
feature parity with the OpenGL backend (`glmd2v`). The current state is a
working Vulkan initialization stack that renders a hardcoded static quad; the
goal is a fully animated, textured MD2 viewer with camera control and an ImGui
UI.

## Current state

- Full Vulkan init: instance, debug messenger, surface, device, swapchain,
  render pass, graphics pipeline, framebuffers, command pool/buffers,
  semaphore/fence sync.
- `BoundBuffer` + staging buffer helpers in `vk/buffer.hpp`.
- `VK::Vertex` has `vec2 pos, vec3 color` — not the MD2 vertex format.
- Renders a hardcoded static quad; no model loading, no texture, no camera,
  no UI.
- `VKEngine::init()` calls only `parse_args()`; no PAK or MD2 loading.
- `include/md2view/vk/ubo.hpp` exists but is empty.

## Phase 1 — Foundation

### 1. Depth buffer
Add a `VK_FORMAT_D32_SFLOAT` depth attachment to `VKEngine`:

- Create image, allocate device-local memory, create image view.
- Add depth attachment to the render pass (`VK_ATTACHMENT_LOAD_OP_CLEAR`,
  layout `VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL`).
- Recreate on swapchain resize (`recreateSwapChain()`).

Without a depth buffer, back faces of the MD2 mesh will overwrite front faces.

### 2. Vertex format
Rewrite `include/md2view/vk/vertex.hpp`:

- `vec3 pos` (world-space, matches `MD2::interpolated_vertices()`)
- `vec2 texcoord` (matches `MD2::scaled_texcoords()`)

Update `getBindingDescription()` and `getAttributeDescriptions()` to match.
Delete the `vec3 color` field.

### 3. UBO / MVP matrices
Populate `include/md2view/vk/ubo.hpp`:

```cpp
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};
```

Add to `VKEngine`:

- Descriptor set layout with binding 0 = uniform buffer.
- One `BoundBuffer` per frame in flight (dynamic, host-visible) for the UBO:
  `kMaxFramesInFlight` of them, **not** one per swapchain image.
- Descriptor pool and one descriptor set per frame in flight.
- Update UBO contents each frame from `Camera` (view) and model matrix.

NB: the UBO is written by the CPU and read by a submission that
`inflightFences_` gates, so it belongs to the frame-in-flight cycle. Only
resources a *presentation* keeps alive belong to the swapchain image cycle
(`renderFinishedSemaphores_`, which are indexed by acquired image index for
exactly this reason). The two counts differ in practice — 2 frames in flight
against 3 swapchain images on the current test hardware — so conflating them
produces a bug that only appears once the cycles drift out of phase.

## Phase 2 — Resource loading

### 4. PAK / MD2 loading
Mirror the GL engine's startup sequence in `VKEngine::init()`:

- Accept `--models-dir` via `parse_args()` (already in base `Engine`).
- Open a `PAK` for the directory.
- Load a default `MD2` model; store `shared_ptr<MD2>` on `VKEngine`.
- Load the first available skin via `PCX`.

### 5. SPIR-V shaders
Write GLSL sources in `data/shaders/`:

- `md2.vert` — inputs: `vec3 pos` (location 0), `vec2 texcoord` (location 1);
  uniform: UBO at binding 0; output: `vec2 fragTexcoord`.
- `md2.frag` — input: `vec2 fragTexcoord`; uniform: `sampler2D` at binding 1;
  output: `vec4 color`.

Add a build step (CMake `add_custom_command` using `glslc`) to compile to
`vert.spv` / `frag.spv` and copy to the build output directory.

## Phase 3 — Per-frame rendering

### 6. Dynamic vertex buffer for MD2 animation
Replace the hardcoded `vertices` / `indices` with live MD2 data:

- Allocate a `BoundBuffer` via `createDynamicVertexBuffer` sized for
  `MD2::vertex_count()` entries.
- Allocate an index buffer from `MD2::triangle_indices()` (static after load).
- Each frame: call `md2_->update(dt)`, map the vertex buffer, write
  `interpolated_vertices()` + `scaled_texcoords()` interleaved as `VK::Vertex`.

### 7. Texture / image
Load the PCX skin decoded pixel buffer into a Vulkan image:

- Create a host-visible staging buffer with the RGBA pixel data.
- Create a `VK_FORMAT_R8G8B8A8_SRGB` device-local image.
- Record and submit a layout transition + buffer-to-image copy.
- Create an image view and a `VkSampler`.
- Add a combined image sampler descriptor (binding 1) to the descriptor set
  layout, pool, and per-frame descriptor sets.

#### 7a. Mipmaps
Required for parity: the GL backend generates a full mip chain
(`glGenerateMipmap`, `src/gl/texture2d.cpp:75`). Vulkan has no equivalent
one-call helper, so the chain has to be built by hand:

- Size the image with `mipLevels = floor(log2(max(width, height))) + 1`, and
  add `vk::ImageUsageFlagBits::eTransferSrc` to its usage — each level is
  blitted *from* the level above it.
- Generate the chain with `vkCmdBlitImage` per level, transitioning level
  `i - 1` from `eTransferDstOptimal` to `eTransferSrcOptimal` before each blit
  and to `eShaderReadOnlyOptimal` after, with the last level transitioned
  separately once the loop ends.
- Check `vk::FormatFeatureFlagBits::eSampledImageFilterLinear` via
  `getFormatProperties()` first. Linear blitting is not guaranteed for every
  format; without it, either generate the chain on the CPU or fall back to a
  single level.
- Sampler: `mipmapMode = eLinear`, `minLod = 0.0f`,
  `maxLod = static_cast<float>(mipLevels)`.

Quake II skins are small — a 200x100 skin yields 8 levels — so the chain costs
very little memory, and the fly-through camera puts models far enough away that
this is visible minification aliasing rather than box-ticking.

## Phase 4 — Camera and input

### 8. GLFW callbacks
Register callbacks on `VKEngine`'s GLFW window:

- `glfwSetKeyCallback` — forward to base `Engine::key_state` as in the GL
  engine.
- `glfwSetCursorPosCallback` — feed `Camera::process_mouse_movement()`.
- `glfwSetScrollCallback` — feed `Camera::process_mouse_scroll()`.

Compute the view matrix from `Camera::view_matrix()` each frame and write it
into the UBO.

## Phase 5 — UI

### 9. ImGui Vulkan backend
Initialize ImGui with `imgui_impl_glfw` + `imgui_impl_vulkan`:

- Create a dedicated `VkDescriptorPool` for ImGui.
- Call `ImGui_ImplVulkan_Init()` with the render pass.
- Each frame: `ImGui_ImplVulkan_NewFrame()` → build panels → record ImGui draw
  commands into the command buffer before `vkQueuePresentKHR`.

The panel code is already shared: `UI::draw(camera)`, `UI::draw(md2)` and the
model selector live in `libmd2ui`, which depends only on `libmd2`, ImGui and
`treehh` — no backend. `vkmd2v` needs to link `libmd2ui` and supply the
*backend* half (`imgui_impl_vulkan`); `GL::Gui` in `src/gl/gui.cpp` is the
OpenGL equivalent. No panel code needs duplicating.

## Phase 6 — Cleanup

### 10. `vk/buffer.hpp` Doxygen style
Convert `/** @brief ... */` comment blocks to `///` triple-slash style to match
the rest of the project.

### 11. Rename `VK::VKEngine` → `VK::Engine`
Mirrors the `GL::Engine` naming convention. Update `include/md2view/vk/engine.hpp`,
`src/vk/engine.cpp`, and `src/vk/main.cpp`.

## Dependency order

```
1 (depth buffer)  ──┐
2 (vertex format) ──┤
3 (UBO)           ──┼──▶  6 (dynamic vbuf) ──┐
4 (PAK/MD2 load)  ──┤                         ├──▶  per-frame draw
5 (shaders)       ──┘                         │
7 (texture)       ────────────────────────────┘
8 (camera)        ──▶  UBO view matrix
9 (ImGui)         ──▶  independent, after 1–8
10, 11            ──▶  cleanup, any time
```

Phases 1–3 yield a correctly rendered static frame. Phase 4 makes it
interactive. Phase 5 adds the full UI. Cleanup items 10 and 11 can land at any
point.
