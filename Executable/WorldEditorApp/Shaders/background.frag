#version 450

layout ( push_constant ) uniform constants
{
    layout(offset = 204) uint draw_mode;
	layout(offset = 208) uvec3 selected_entity;
	layout(offset = 220) uint highlight_enabled;
	layout(offset = 224) vec4 colorMask;
} ObjectPicking;

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform samplerCube skybox;

 void main() 
 {
	outColor = texture( skybox, fragTexCoord ) * vec4(1 - ObjectPicking.draw_mode);
 }
