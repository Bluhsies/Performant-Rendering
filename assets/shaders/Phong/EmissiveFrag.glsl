#version 460 core

layout(location = 0) out vec4 colour;

in vec4 fragmentPosLightSpace;
in vec3 normal;
in vec3 fragmentPos;
in vec2 texCoord;

struct directionalLight
{
	vec3 colour;
	vec3 direction;
};

struct pointLight
{
	vec3 colour;
	vec3 position;
	vec3 constants;
};

struct spotLight
{
	vec3 colour;
	vec3 position;
	vec3 direction;
	vec3 constants;
	float cutOff;
	float outerCutOff;
};

const int numPointLights = 1;
const int numSpotLights = 1;

layout (std140, binding = 1) uniform b_lights
{
	uniform directionalLight dLight;
	uniform pointLight pLights[numPointLights];
	uniform spotLight sLights[numSpotLights];
};

layout (std140, binding = 0) uniform b_camera
{
	uniform mat4 u_view;
	uniform mat4 u_projection;
	uniform vec3 u_viewPos;
};

uniform vec3 u_albedo;
uniform vec4 u_emissive;
uniform sampler2D u_albedoMap;

vec3 getDirectionalLight()
{
	float ambientStrength = 0.4;
	vec3 ambient = ambientStrength * dLight.colour;
	float diff = max(dot(normal, -dLight.direction), 0.0);
	vec3 diffuse = diff * dLight.colour;
	float specularStrength = 0.8;
	vec3 viewDir = normalize(u_viewPos - fragmentPos);
	vec3 reflectDir = reflect(dLight.direction, normal);  
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64);
	vec3 specular = specularStrength * spec * dLight.colour;      
	return ambient * (diffuse + specular);
}

void main()
{   
	vec3 emissive = u_emissive.rgb;
	float strength = u_emissive.a;
	
	vec3 result = getDirectionalLight();
	
	vec4 albedo = vec4(result * u_albedo, 1.0) * texture(u_albedoMap, texCoord);
	colour = albedo + vec4(emissive, 1.0) * strength;
}
