#version 330

uniform sampler2D texture0;

in vec2 TexCoord;
flat in int pass;

out vec4 color;

void main(void) {

  if (1 == pass) {
    color = vec4(1.0, 0.0, 0.0, 1.0);
  }
  else {
    color = texture(texture0, TexCoord);
  }
}
