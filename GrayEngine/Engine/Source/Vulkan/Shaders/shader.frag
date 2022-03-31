#version 450

layout(location = 0) in vec4 colorMask;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler[5];

void main() 
{
	vec4 accum = vec4(0.0);
	for (int i = 0; i < 5; i++)
	{
		accum += texture(texSampler[i], fragTexCoord);
	}
	
	outColor = colorMask * accum;
}