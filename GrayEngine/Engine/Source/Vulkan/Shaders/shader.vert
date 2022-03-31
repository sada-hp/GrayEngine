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

layout(location = 0) out vec4 colorMask;
layout(location = 1) out vec2 fragTexCoord;

void main() 
{
    gl_Position = PushConst.proj * PushConst.view * PushConst.model * inPosition;
    colorMask = inColor;
	fragTexCoord = inTexCoord;
}