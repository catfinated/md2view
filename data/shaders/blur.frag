#version 330 core

in vec2 TexCoords;
flat in int no_blur;
out vec4 color;

uniform sampler2D screenTexture;

vec4 blur();
vec4 kernel_effect(float kernel[9]);

void main()
{
  if (no_blur == 1) {
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

  return kernel_effect(kernel);
}

vec4 kernel_effect(float kernel[9])
{
  const float offset = 1.0 / 300;
  vec2 offsets[9] = vec2[](vec2(-offset, offset),
                           vec2(0.0f, offset),
                           vec2(offset, offset),
                           vec2(-offset, 0.0f),
                           vec2(0.0f, 0.0f),
                           vec2(offset, 0.0f),
                           vec2(-offset, -offset),
                           vec2(0.0f, -offset),
                           vec2(offset, -offset));


  vec3 sampleTex[9];
  for (int i = 0; i < 9; ++i) {
    sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
  }

  vec3 col = vec3(0.0);
  for (int i = 0; i < 9; ++i) {
    col += sampleTex[i] * kernel[i];
  }

  return vec4(col, 1.0);
}
