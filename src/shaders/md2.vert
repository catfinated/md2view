#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(void)
{
  //TexCoord = vec2(texCoord.x, 1.0f - texCoord.y);
  TexCoord = texCoord; //vec2(texCoord.x, texCoord.y);
  gl_Position = projection * view * model * vec4(position, 1.0);
}
