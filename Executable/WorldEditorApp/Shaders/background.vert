#version 450

layout ( push_constant ) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
} PushConst;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in uint inUvIndex;

layout(location = 0) out vec4 colorMask;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint mapIndex;


vec3 gridPlane[4] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, -1, 0)
);

void main() 
{
	vec3 p = gridPlane[gl_VertexIndex].xyz;
	fragTexCoord = inTexCoord;
	mapIndex = inUvIndex;
    gl_Position = vec4(p, 1.0);
}