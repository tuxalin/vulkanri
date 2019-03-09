#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform Lights {
    vec4 lights[4];
    float ambient;
    float exposure;
} lightParams;
layout (binding = 2) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outColor;

void main() 
{
    vec3 color = texture(samplerCubeMap, inUVW).rgb;
    color = vec3(1.0) - exp(-color * lightParams.exposure);
    outColor = vec4(color.rgb, 1.0);
}
