#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D screenTexture;
uniform int disable_blur;

vec4 blur();

void main()
{
  if (disable_blur == 1) {
    color = texture(screenTexture, TexCoords);
  }
  else {
    color = blur();
  }
}

vec4 blur()
{
  float kernel[9] = float[](1.0 / 16, 2.0 / 16, 1.0 / 16,
                            2.0 / 16, 4.0 / 16, 2.0 / 16,
                            1.0 / 16, 2.0 / 16, 1.0 / 16);

  const float offset = 1.0 / 256;
  vec2 offsets[9] = vec2[](vec2(-offset, offset),
                           vec2(0.0f, offset),
                           vec2(offset, offset),
                           vec2(-offset, 0.0f),
                           vec2(0.0f, 0.0f),
                           vec2(offset, 0.0f),
                           vec2(-offset, -offset),
                           vec2(0.0f, -offset),
                           vec2(offset, -offset));

  vec3 col = vec3(0.0);
  for (int i = 0; i < 9; ++i) {
    col += vec3(texture(screenTexture, TexCoords.st + offsets[i])) * kernel[i];
  }

  return vec4(col, 1.0);
}
