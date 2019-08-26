#version 330

uniform sampler2D texture0;

in vec2 Frag_UV;
in vec4 Frag_Color;

out vec4 color;

void main()
{
  color = Frag_Color * texture(texture0, Frag_UV.st);
}
