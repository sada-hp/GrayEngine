#version 450

layout ( push_constant ) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
} PushConst;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;


void main() {
    gl_Position = PushConst.proj * PushConst.view * PushConst.model * inPosition;
    fragColor = inColor;
}