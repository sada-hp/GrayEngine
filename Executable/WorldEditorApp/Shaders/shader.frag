#version 450

layout(location = 0) in vec4 colorMask;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint mapIndex;
layout(location = 3) in flat uint useTexturing;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D texSampler[5];

void main() 
{
	if (useTexturing > 0)
	{
		vec4 accum = vec4(0.0);
		for (int i = 0; i < 5; i++)
		{
			if (i == mapIndex)
			{
				accum += texture(texSampler[i], fragTexCoord);
				accum *= vec4(colorMask.xyz, 1.f);
			}
		}
	
		outColor = vec4(min(1.0, accum.x), min(1.0, accum.y),min(1.0, accum.z),min(1.0, accum.w));
	}
	else
	{
		outColor = colorMask;
	}
}