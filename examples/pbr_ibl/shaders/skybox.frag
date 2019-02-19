#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "../../resources/shaders/pbr.glsl"

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

void main() 
{
	vec3 color = texture(samplerCubeMap, inUVW).rgb;
	color = gammaCorrection(color);	
	outColor = vec4(color.rgb, 1.0);
}
