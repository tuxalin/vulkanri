#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera {
	vec4 worldPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 mvp;
} uboCamera;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec2 outFragTexCoord;
layout(location = 1) out vec4 outWorldPos;
layout(location = 2) out vec3 outNormal;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() 
{    
	outWorldPos = uboCamera.model * vec4(inPosition, 0.0, 1.0);
	outNormal = mat3(uboCamera.model) * inNormal;
    outFragTexCoord = inTexCoord;
    gl_Position = uboCamera.mvp * vec4(inPosition, 0.0, 1.0);
}
