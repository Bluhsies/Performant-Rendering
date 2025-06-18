#version 450 core
			
layout(location = 0) out vec4 colour;

in vec2 texCoord;

uniform sampler2D u_albedoMap;
uniform float u_threshold;

void main()
{
	vec3 rgb = textureLod(u_albedoMap, texCoord, 0).rgb;
	float brightness = dot(rgb, vec3(0.2126, 0.7152, 0.0722));

	colour = mix(vec4(0.0, 0.0, 0.0, 1.0), vec4(rgb,1), step(u_threshold, brightness));
}