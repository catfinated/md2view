#version 330 core

in vec2 TexCoords;
out vec4 color;

uniform sampler2D screenTexture;
uniform sampler2D prepassTexture;
uniform sampler2D blurredTexture;

void main()
{
  // no effects
  vec4 col = texture(screenTexture, TexCoords);
  vec4 blurred = texture(blurredTexture, TexCoords);
  vec4 prepass = texture(prepassTexture, TexCoords);
  vec4 glow = max(vec4(0), blurred - prepass);
  color = col + glow * 2.0;
}
