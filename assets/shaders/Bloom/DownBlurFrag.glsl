#version 450 core
			
layout(location = 0) out vec4 colour;

in vec2 texCoord;

uniform sampler2D u_albedoMap;
uniform vec2 u_UVperPixel;

void main()
{
	vec3 pixel = vec3(0.f, 0.f, 0.f);
  
	float GaussianKernel[25] = {
		1.f / 256.f, 4.f  / 256.f,  6.f / 256.f,  4.f / 256.f, 1.f / 256.f,
		4.f / 256.f, 16.f / 256.f, 24.f / 256.f, 16.f / 256.f, 4.f / 256.f,
		6.f / 256.f, 24.f / 256.f, 36.f / 256.f, 24.f / 256.f, 6.f / 256.f,
		4.f / 256.f, 16.f / 256.f, 24.f / 256.f, 16.f / 256.f, 4.f / 256.f,
		1.f / 256.f, 4.f  / 256.f,  6.f / 256.f,  4.f / 256.f, 1.f / 256.f
	};

	for(int i = 0 ; i < 5; i++) {
		for(int j = 0 ; j < 5; j++) {
			vec2 offset;
			offset.x = (i-2) * u_UVperPixel.x;
			offset.y = (j-2) * u_UVperPixel.y;
			vec3 rgb = texture(u_albedoMap, texCoord + offset).rgb;
			pixel += rgb * GaussianKernel[i + j * 5];
		}
	}
	colour = vec4(pixel, 1.0);
}