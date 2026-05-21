# md2view Design

md2view is a Quake II MD2 model viewer written in C++23. It supports two
rendering backends (OpenGL and Vulkan) and is structured so that model loading,
animation, and GPU rendering are independent concerns.

## Layer architecture

```
┌─────────────────────────────────────────────────────┐
│  Application (MD2View / GLEngine / VKEngine)        │
├─────────────────────────────────────────────────────┤
│  Scene composition                                  │
│  MD2View owns MD2 + GL::Mesh; wires update → sync  │
├──────────────────────┬──────────────────────────────┤
│  Data / logic layer  │  GPU layer                   │
│  MD2                 │  GL::Mesh  (or future VK::Mesh)│
│  PAK  PCX            │                              │
└──────────────────────┴──────────────────────────────┘
```

There are three layers:

**Data / logic layer** — pure C++, no graphics API dependency. Parses file
formats and owns all model state.

**GPU layer** — owns graphics API objects (VAO/VBO for GL, buffers for VK).
Knows nothing about model types; operates on raw vertex and texcoord spans.

**Application / scene layer** — owns one instance of each and wires them
together: advances model animation, syncs updated vertex data to the GPU, then
issues draw calls.

## Data layer

### PAK (`pak.hpp`)
Quake II PAK archive reader. Supports both real `.pak` files (binary archive
with a flat directory) and plain directories treated as a PAK (used during
development). Exposes a uniform `open_ifstream(path)` interface so callers do
not need to distinguish between the two.

### PCX (`pcx.hpp`)
Loads PCX images used as model skins. Decodes the 128-byte header, RLE-encoded
scan lines, and the 256-entry VGA palette into an in-memory RGB image buffer.
Has no graphics API dependency; the caller is responsible for uploading the
pixel data to a texture.

### MD2 (`md2.hpp`)
Parses the MD2 model format and owns all animation state. MD2 is a keyframe
animation format: each frame stores compressed vertex positions which the loader
unpacks and scales into world-space `glm::vec3` values.

Responsibilities:
- Parsing: header, skins, triangle list, texture coordinates, frames
- Coordinate unpacking: scales `uint8` vertex components by per-frame scale and
  translate vectors; remaps axes to match the GL coordinate system
- Texture coordinate scaling: divides raw integer ST values by `skinwidth` /
  `skinheight` and unpacks the triangle list into a flat buffer (one entry per
  triangle vertex) for use with `glDrawArrays`
- Animation state machine: tracks current/next frame indices and a fractional
  interpolation value; `update(dt)` lerps between keyframes and writes the
  result into `interpolated_vertices_`

**MD2 has no OpenGL dependency.** It exposes the current frame data through two
const accessors:

```cpp
std::vector<glm::vec3> const& interpolated_vertices() const;
std::vector<glm::vec2> const& scaled_texcoords() const;
```

These are the only values the GPU layer needs. Because MD2 does not touch any
graphics API, it can be constructed and tested in unit tests without a GL or
Vulkan context.

## GPU layer

### GL::Mesh (`gl/mesh.hpp`)
Owns a VAO and two VBOs for one triangulated mesh. It is not coupled to any
model type; it operates purely on `std::span<glm::vec3>` and
`std::span<glm::vec2>`.

```cpp
// Allocate GPU resources and upload initial data
GL::Mesh mesh{model.interpolated_vertices(), model.scaled_texcoords()};

// Re-upload vertex data after animation update (texcoords are static)
mesh.sync(model.interpolated_vertices());

// Issue draw call
mesh.draw(shader);
```

Adding a new model format (MD3, OBJ, etc.) does not require a new renderer
class. Any model that produces a flat `span<vec3>` of vertex positions and a
`span<vec2>` of texture coordinates is compatible with `GL::Mesh`.

A future `VK::Mesh` would follow the same interface using Vulkan device-local
buffers and staging uploads, with `sync()` recording a transfer command.

## Scene composition (MD2View)

`MD2View` owns both the data and GPU objects and is responsible for keeping them
in sync each frame:

```
update(dt):
    md2_->update(dt);               // advance animation, write interpolated_vertices_
    md2_mesh_->sync(md2_->interpolated_vertices()); // upload to GPU

render():
    md2_mesh_->draw(*shader_);      // glDrawArrays
```

When the user selects a different model, `load_model()` replaces both `md2_`
and `md2_mesh_`. `ResourceManager` caches `MD2` objects by path and is unaware
of the GPU layer.

## Rendering pipeline (GL backend)

The GL backend uses a two-pass approach with framebuffer objects:

1. **Main pass** — renders the model into a multi-target FBO: colour attachment 0
   holds the lit scene, colour attachment 1 holds a mask of glowing surfaces.
2. **Blur pass** — applies a separable blur to the glow mask.
3. **Composite pass** — blends the lit scene, glow mask, and blurred glow using
   a full-screen quad shader.

The glow effect can be toggled at runtime from the ImGui panel.

## Testing

Unit tests cover the data layer only (no GL context required):

- **PAK**: directory mode, archive mode, magic validation, entry streaming
- **PCX**: header parsing, palette decoding, pixel decode with exact RGBA values
- **MD2**: header field validation, animation name parsing, vertex count,
  vertex positions after coordinate unpacking, texture coordinate scaling

Test fixtures are generated at build time by `tests/gen_fixtures.cpp`, a
standalone program with no project dependencies. See `tests/README.md` for
details.
