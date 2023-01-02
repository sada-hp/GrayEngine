#version 450

layout ( push_constant ) uniform constants
{
    layout(offset = 204) uint draw_mode;
	layout(offset = 208) uvec3 selected_entity;
	layout(offset = 220) uint highlight_enabled;
	layout(offset = 224) vec4 colorMask;
} ObjectPicking;

layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint mapIndex;
layout(location = 3) in flat uint useTexturing;
layout(location = 4) in flat uvec3 fragID;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2DArray texSampler;

void main() 
{
	if (ObjectPicking.draw_mode == 0)
	{
		if (useTexturing > 0)
		{
			outColor = texture(texSampler, vec3(fragTexCoord, mapIndex)) * ObjectPicking.colorMask;
		}
		else
		{
			outColor = ObjectPicking.colorMask;
		}
		
		if (ObjectPicking.selected_entity == fragID && ObjectPicking.highlight_enabled > 0)
		{
			outColor += vec4(0.1, 0.1, 0.5, outColor.w);
		}
	}
	else
	{
		float r = fragID.x / 255.f;
		float g = fragID.y / 255.f;
		float b = fragID.z / 255.f;
		outColor = vec4(r, g, b, 1);
	}
}