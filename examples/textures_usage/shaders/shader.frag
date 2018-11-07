#version 450
#extension GL_ARB_separate_shader_objects : enable
 
#include "../../resources/shaders/pbr.glsl"
#include "../../resources/shaders/normals.glsl"

layout(binding = 0) uniform Camera {
	vec4 worldPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} camera;

layout(binding = 5) uniform Lights {
	vec4 lights[4];
	float ambient;
} lightParams;

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D roughnessMap;
layout(binding = 4) uniform sampler2D ambientOcclusionMap;
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

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

const vec3 lightColor = vec3(1.0);

void main() 
{
	vec3 albedo = vec3(material.r, material.g, material.b) * gammaDecode(texture(albedoMap, inUV).rgb);
	float roughness = material.roughness * texture(roughnessMap, inUV).r;
	float specular = material.specular;
	float metallic = material.metallic;
	float ao = mix(1.0, texture(ambientOcclusionMap, inUV).r, material.aoStrength);

	vec3 normal = computeSurfaceNormal(inNormal, inWorldPos, normalMap, inUV);
	normal = normalize(normal * vec3(material.normalStrength, material.normalStrength, 1.0));

	vec3 N = normalize(normal);
	vec3 V = normalize(camera.worldPos.xyz - inWorldPos);

	// reflectance 
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < lightParams.lights.length(); i++) 
	{
		vec3 L = normalize(lightParams.lights[i].xyz - inWorldPos);
		vec3 radiance = lightColor * lightParams.lights[i].a;
		vec3 ct = cookTorrance(L, V, N, albedo, metallic, specular, roughness) * radiance;
		Lo += ct * diffuseLambert(L, N); 
	};

	vec3 ambient = albedo * lightParams.ambient;
	vec3 color = (ambient + Lo) * ao;

	color = color / (color + vec3(1.0)); // HDR tonemapping
	color = gammaEncode(color);

    outColor = vec4(color.rgb, 1.0);
}
