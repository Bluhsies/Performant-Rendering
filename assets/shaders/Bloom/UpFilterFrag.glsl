#version 450 core
			
layout(location = 0) out vec4 colour;

in vec2 texCoord;

uniform sampler2D u_toFilter;
uniform sampler2D u_toAddTo;
uniform vec2 u_filterUVperPixel;
uniform float u_filterScalar;
uniform float u_addToScalar;

void main()
{
	vec3 filteredPixel = vec3(0.f, 0.f, 0.f);
	float tentKernel[9] = {
		1.f/16.f, 2.f/16.f, 1.f/16.f,
		2.f/16.f, 4.f/16.f, 2.f/16.f,
		1.f/16.f, 2.f/16.f, 1.f/16.f
	};

	for(int i = 0 ; i < 3; i++) {
		for(int j = 0 ; j < 3; j++) {
			vec2 offset;
			offset.x = (i-1) * u_filterUVperPixel.x;
			offset.y = (j-1) * u_filterUVperPixel.y;
			vec3 rgb = texture(u_toFilter, texCoord + offset).rgb;
			filteredPixel += rgb * tentKernel[i + j * 3];
		}
	}

	vec3 pixel = texture(u_toAddTo, texCoord).rgb; 

	colour = vec4((filteredPixel * u_filterScalar) + (pixel * u_addToScalar), 1.0);
}