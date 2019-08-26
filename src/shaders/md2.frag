#version 330

uniform sampler2D texture0;

in vec2 TexCoord;

out vec4 color;

void main(void) {
  color = texture(texture0, TexCoord);
  // color = vec4(1.0, 0.0, 0.0, 1.0);
}
