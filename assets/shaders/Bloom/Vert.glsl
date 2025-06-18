#version 450 core
			
layout(location = 0) in vec3 a_vertexPosition;
layout(location = 1) in vec2 a_texCoord;

out vec2 texCoord;

uniform mat4 u_projection;

void main()
{
	texCoord = a_texCoord;
	gl_Position =  u_projection * vec4(a_vertexPosition,1.0);
}