#version 330

uniform sampler2D skin;

in vec2 TexCoords;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 glow;

uniform vec3 glow_color;

void main(void) {

  glow = vec4(glow_color, 1.0);
  color = texture(skin, TexCoords);
}
