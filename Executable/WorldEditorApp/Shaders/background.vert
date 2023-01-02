#version 450

layout ( push_constant ) uniform constants
{
    mat4 model;
    mat4 view;
    mat4 proj;
} PushConst;

layout(location = 0) in vec4 inPosition;

layout(location = 1) out vec3 fragTexCoord;

void main() 
{
    gl_Position = (PushConst.proj * vec4( mat3(PushConst.view) * inPosition.xyz, 0.0 )).xyzz;
	fragTexCoord = inPosition.xyz;
}