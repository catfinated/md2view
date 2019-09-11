#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void)
{
  TexCoords = texCoords;
  gl_Position = projection * view * model * vec4(position, 1.0);
}
