#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;

layout(binding = 0) uniform Camera {
	vec4 worldPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;

layout (location = 0) out vec3 outUVW;

void main() 
{
	vec4 pos = vec4(inPos * 2.0, 1.0); // from 0.5
	outUVW = pos.xyz; 

	mat4 view = mat4(mat3(camera.view));
	pos = camera.proj * view * pos;
	gl_Position = pos.xyww;
}
