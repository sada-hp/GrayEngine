//With a help of http://asliceofrendering.com/

#version 450

layout ( push_constant ) uniform constants
{
    layout(offset = 204) uint draw_mode;
	layout(offset = 208) uvec3 selected_entity;
	layout(offset = 220) uint highlight_enabled;
	layout(offset = 224) vec4 colorMask;
} ObjectPicking;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;
layout(location = 2) in mat4 fragView;
layout(location = 6) in mat4 fragProj;

layout(location = 0) out vec4 outColor;

const float near = 0.1;
const float far = 100;
int axis = 0;

vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.25, 0.25, 0.25, 1.0 - min(line, 1.0));
    if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
	{
        color.x = 0.0;
        color.y = 0.0;
        color.z = 1.0;
		axis = 1;
	}
    if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
	{
        color.x = 1.0;
        color.y = 0.0;
        color.z = 0.0;
		axis = 1;
	}
    return color;
}
float computeDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}
float computeLinearDepth(vec3 pos) {
    vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linearDepth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linearDepth / far; // normalize
}
void main() {
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    gl_FragDepth = 0.999999;

    float linearDepth = computeLinearDepth(fragPos3D);
    float fading = max(0, (0.5 - linearDepth));
	
    outColor = (((grid(fragPos3D, 10) * max(axis, (0.75 - linearDepth)) + grid(fragPos3D, 1)) * max(axis, (0.35 - linearDepth))) * float(t > 0)) * vec4(1 - ObjectPicking.draw_mode);
}