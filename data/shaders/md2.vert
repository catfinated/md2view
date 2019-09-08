#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;
flat out int pass;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int render_pass;

void main(void)
{
  TexCoord = texCoord;
  pass = render_pass;
  gl_Position = projection * view * model * vec4(position, 1.0);
}
