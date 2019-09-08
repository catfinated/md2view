#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;
flat out int no_blur;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int disable_blur;

void main()
{
  no_blur = disable_blur;
  gl_Position = vec4(position.x, position.y, 0.0f, 1.0f);
  TexCoords = texCoords;
}
