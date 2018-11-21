#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera {
	vec4 worldPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

void main(void)
{
    outUV = inUV;
	outNormal = normalize(mat3(camera.model) * inNormal);

	gl_Position = camera.model * vec4(inPos.xyz, 1.0);
}