#version 450

layout (depth_greater) out float gl_FragDepth;
layout(location = 0) in vec4 colorMask;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat int mapIndex;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler[5];

void main() 
{
	vec4 accum = vec4(0.0);
	for (int i = 0; i < 5; i++)
	{
		if (i == mapIndex)
		{
			accum += texture(texSampler[i], fragTexCoord);
		}
	}
	
	outColor = vec4(accum.xyz, min(1.0, accum.w));
	
	gl_FragDepth = 1;
}