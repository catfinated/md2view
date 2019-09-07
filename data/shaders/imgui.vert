#version 330

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 color;

uniform mat4 projection;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
  Frag_UV = uv;
  Frag_Color = color;
  gl_Position = projection * vec4(position.xy, 0, 1);
}
