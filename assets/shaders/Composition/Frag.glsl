#version 450 core
			
layout(location = 0) out vec4 colour;

in vec2 texCoord;

uniform sampler2D u_albedoMap;
uniform sampler2D u_bloomMap;

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
	// Add output from main pass to Bloom texture
	vec3 rgb = texture(u_albedoMap, texCoord).rgb + texture(u_bloomMap, texCoord).rgb;
	
	// Tone map with aces
	rgb = aces(rgb);

	// Gamma correction
	rgb = pow(rgb, vec3(1.0/2.2));

	colour = vec4(rgb,1.0);
}