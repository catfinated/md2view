#version 330

layout (location = 0) in vec3 a_Vertex;
layout (location = 1) in vec2 a_TexCoord0;

out vec2 texCoord0;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main(void) 
{
	vec4 pos = model * vec4(a_Vertex, 1.0);	
	//texCoord0 = a_TexCoord0;
   texCoord0 = vec2(a_TexCoord0.x, a_TexCoord0.y);
	gl_Position = projection * view * pos;	
}
