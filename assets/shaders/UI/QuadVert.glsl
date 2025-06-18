#version 450 core
// Programmable Vertex Pulling
// Based on 3D Graphics Rendering Cookbook Edition 1 Chapter 3 pp.89-94 

// Structs

struct Vertex { 
	float pos[2];
	float tc[2];
	float tint[4];
	float texSlot;
};

struct VertexOutput {
	vec2 texCoord;
	vec4 tint;
	float texSlot;
};

// Buffers

layout(std430, binding = 0) readonly buffer vertexSSBO {
    Vertex vertices[ ]; 
};

layout (std140, binding = 3) uniform b_cameraUI {
	uniform mat4 u_view;
	uniform mat4 u_projection;
};

// PVP functions

vec2 getPosition(int i) {
	return vec2(vertices[i].pos[0], vertices[i].pos[1]);
}

vec2 getTexCoord(int i) {
	return vec2(vertices[i].tc[0], vertices[i].tc[1]);
}

vec4 getTint(int i) {
	return vec4(vertices[i].tint[0], vertices[i].tint[1], vertices[i].tint[2], vertices[i].tint[3]);
}

float getTexSlot(int i) {
	return vertices[i].texSlot;
}

// Shader

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.texCoord = getTexCoord(gl_VertexID);
	Output.texSlot = getTexSlot(gl_VertexID);
	Output.tint = getTint(gl_VertexID);
	vec2 pos = getPosition(gl_VertexID);
	gl_Position =  u_projection * u_view * vec4(pos,1.0,1.0);
}