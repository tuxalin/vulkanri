#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Camera {
	vec4 worldPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;
layout(binding = 6) uniform Material {
	float roughness;
	float metallic;
	float specular;
	float r;
	float g;
	float b;
	float normalStrength;
	float aoStrength;
	float displacementStrength;
	float tessLevel;
} material;
layout (binding = 7) uniform sampler2D displacementMap; 

layout(triangles, equal_spacing, cw) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outWorldPos; 
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;

vec2 interpolate2D(vec2 val1, vec2 val2, vec2 val3)
{
	return gl_TessCoord.x * val1 + gl_TessCoord.y * val2 + gl_TessCoord.z * val3;
}

vec3 interpolate3D(vec3 val1, vec3 val2, vec3 val3)
{
	return gl_TessCoord.x * val1 + gl_TessCoord.y * val2 + gl_TessCoord.z * val3;
}

void main()
{
	vec3 worldPos = interpolate3D(gl_in[0].gl_Position.xyz, gl_in[1].gl_Position.xyz, gl_in[2].gl_Position.xyz);
	outNormal = interpolate3D(inNormal[0], inNormal[1], inNormal[2]); 
	outUV = interpolate2D(inUV[0], inUV[1], inUV[2]);

	float height = max(textureLod(displacementMap, outUV.st, 0.0).r, 0.0);
	float strength = height * height * material.displacementStrength;
	worldPos += normalize(outNormal) * strength;
	outWorldPos = worldPos;
		
	gl_Position = camera.viewProj * vec4(worldPos, 1.0);
}
