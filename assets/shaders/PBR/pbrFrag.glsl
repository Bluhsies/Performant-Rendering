#version 460 core

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


const int numPointLights = 2;
const int numSpotLights = 2;

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


///////////////////////// INS OUTS
out vec4 FragColour;

in vec2 UV ;
in vec3 norm ;
in vec3 posInWS ;
in mat3 TBN;


///////////////////////// UNIFORMS

uniform float u_ambientFactor;
uniform float u_metallic;
uniform float u_roughness;


uniform sampler2D albedoTexture;
uniform sampler2D normalTexture;
uniform sampler2D roughTexture;
uniform sampler2D metalTexture;
uniform sampler2D aoTexture;

///////////////////////// FUNCTIONS

vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0);
float GeometrySchlickGGX(float NdotV);
float DistributionGGX();
float GeometrySmith();


///////////////////////// Globals

const float PI = 3.14159265 ;
float NdotL ;
float NdotV ;
float NdotH ;

float alpha ;
float metal ;
float AO ;


///////////////////////// MAIN

void main()
{    
    vec2 uv = UV ; 
    vec3 N = texture(normalTexture, uv).rgb;
    N = normalize(TBN * (N * 2.0 - 1.0));
   
    vec3 alb = texture(albedoTexture, uv).rgb;
    alpha = texture(roughTexture, uv).r;
    metal = texture(metalTexture, uv).r  ;    

    AO = texture(aoTexture, uv).r;
    vec3 F0 = mix(vec3(0.4), alb, metal); 


    vec3 V = normalize(u_viewPos - posInWS);    // View Direction
    vec3 L = normalize(-dLight.direction);      // Light Direction
    vec3 H = normalize(L + V);                // Half-way vector

      

    NdotL = max(dot(N, L), 0.0);              // cache these calculations
    NdotV = max(dot(N, V), 0.0);
    NdotH = max(dot(N, H), 0.0);


    /////////////////////
    // Cook-Torrance BRDF
    float D = DistributionGGX();   
    float G = GeometrySchlickGGX(NdotV) * GeometrySchlickGGX(NdotL) ; 
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
    vec3 numerator    = D * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;
        
    // kS is equal to Fresnel
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metal;	  
    vec3 Lo = (kD * alb / PI + specular) * NdotL;  
    vec3 ambient = vec3(0.03) * alb * AO;
    
    vec3 color = ambient + Lo;

    FragColour = vec4(color, 1.0);
}


float DistributionGGX()
{
    float a = alpha * alpha;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return a2 / denominator;
}

float GeometrySchlickGGX(float Ndot)
{
    float r = (alpha + 1.0);
    float k = (r * r) / 8.0;
    float denominator = Ndot * (1.0 - k) + k;

    return Ndot / denominator;
}

float GeometrySmith()
{
    float ggx2 = GeometrySchlickGGX(NdotV);
    float ggx1 = GeometrySchlickGGX(NdotL);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0)
{
    return F0 + (max(vec3(1.0 - alpha), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  
