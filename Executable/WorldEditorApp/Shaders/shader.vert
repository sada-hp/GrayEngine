#version 450

layout ( push_constant ) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
	vec3 scale;
} PushConst;

layout(location = 0) in vec4 inPosition;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inUvIndex;
layout(location = 4) in uint usesTexture;
layout(location = 5) in uvec3 inID;

layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint mapIndex;
layout(location = 3) out uint useTexturing;
layout(location = 4) out uvec3 fragID;

void main() 
{
    gl_Position = PushConst.proj * PushConst.view * PushConst.model * (inPosition*vec4(PushConst.scale, 1));
	fragTexCoord = inTexCoord;
	mapIndex = inUvIndex;
	useTexturing = usesTexture;
	fragID = inID;
}