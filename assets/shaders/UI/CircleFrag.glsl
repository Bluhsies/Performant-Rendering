#version 450 core
// Circle drawing based on https://github.com/TheCherno/Hazel/blob/text/Hazelnut/assets/shaders/Renderer2D_Circle.glsl

struct VertexOutput {
	vec2 unitPosition;
	vec4 tint;
	float thickness;
};

layout(location = 0) out vec4 colour;

layout (location = 0) in VertexOutput Input;

void main()
{
	float distance = 1.0 - length(Input.unitPosition);
    float circle = smoothstep(0.0, 0.05, distance);
	circle *= smoothstep(Input.thickness + 0.05, Input.thickness, distance);

	if (circle == 0.0) discard;

	vec4 result = Input.tint;
	colour = result;
}