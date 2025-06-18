#version 450 core
			
layout(location = 0) out vec4 colour;

struct VertexOutput {
	vec2 texCoord;
	vec4 tint;
	float texSlot;
};

layout (location = 0) in VertexOutput Input;

uniform sampler2D u_textureSlots[32];

void main()
{
	vec4 result = Input.tint;
	switch(int(Input.texSlot))
	{
		case  0: result *= texture(u_textureSlots[ 0], Input.texCoord); break;
		case  1: result *= texture(u_textureSlots[ 1], Input.texCoord); break;
		case  2: result *= texture(u_textureSlots[ 2], Input.texCoord); break;
		case  3: result *= texture(u_textureSlots[ 3], Input.texCoord); break;
		case  4: result *= texture(u_textureSlots[ 4], Input.texCoord); break;
		case  5: result *= texture(u_textureSlots[ 5], Input.texCoord); break;
		case  6: result *= texture(u_textureSlots[ 6], Input.texCoord); break;
		case  7: result *= texture(u_textureSlots[ 7], Input.texCoord); break;
		case  8: result *= texture(u_textureSlots[ 8], Input.texCoord); break;
		case  9: result *= texture(u_textureSlots[ 9], Input.texCoord); break;
		case 10: result *= texture(u_textureSlots[10], Input.texCoord); break;
		case 11: result *= texture(u_textureSlots[11], Input.texCoord); break;
		case 12: result *= texture(u_textureSlots[12], Input.texCoord); break;
		case 13: result *= texture(u_textureSlots[13], Input.texCoord); break;
		case 14: result *= texture(u_textureSlots[14], Input.texCoord); break;
		case 15: result *= texture(u_textureSlots[15], Input.texCoord); break;
		case 16: result *= texture(u_textureSlots[16], Input.texCoord); break;
		case 17: result *= texture(u_textureSlots[17], Input.texCoord); break;
		case 18: result *= texture(u_textureSlots[18], Input.texCoord); break;
		case 19: result *= texture(u_textureSlots[19], Input.texCoord); break;
		case 20: result *= texture(u_textureSlots[20], Input.texCoord); break;
		case 21: result *= texture(u_textureSlots[21], Input.texCoord); break;
		case 22: result *= texture(u_textureSlots[22], Input.texCoord); break;
		case 23: result *= texture(u_textureSlots[23], Input.texCoord); break;
		case 24: result *= texture(u_textureSlots[24], Input.texCoord); break;
		case 25: result *= texture(u_textureSlots[25], Input.texCoord); break;
		case 26: result *= texture(u_textureSlots[26], Input.texCoord); break;
		case 27: result *= texture(u_textureSlots[27], Input.texCoord); break;
		case 28: result *= texture(u_textureSlots[28], Input.texCoord); break;
		case 29: result *= texture(u_textureSlots[29], Input.texCoord); break;
		case 30: result *= texture(u_textureSlots[30], Input.texCoord); break;
		case 31: result *= texture(u_textureSlots[31], Input.texCoord); break;
	}
	colour = result;
}