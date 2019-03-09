#version 450
#extension GL_ARB_separate_shader_objects : enable
 
#include "../../resources/shaders/pbr.glsl"
#include "../../resources/shaders/gamma.glsl"
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
    float exposure;
} lightParams;

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D metallicRoughnessMap;
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
} material;
layout(binding = 7) uniform samplerCube irradianceMap;
layout(binding = 8) uniform samplerCube prefilteredMap;
layout(binding = 9) uniform sampler2D brdfLUT;

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outColor;

const vec3 lightColor = vec3(1.0);
const float lightAttenuation = 500.0; // unit distance

void main() 
{
    vec3 albedo = vec3(material.r, material.g, material.b) * gammaDecode(texture(albedoMap, inUV).rgb);
    vec2 metallicRoughness = texture(metallicRoughnessMap, inUV).gb;
    float roughness = max(material.roughness * metallicRoughness.x, 0.04);
    float specular = material.specular;
    float metallic = material.metallic * metallicRoughness.y;
    float ao = mix(1.0, texture(ambientOcclusionMap, inUV).r, material.aoStrength);

    vec3 N = computeSurfaceNormal(inNormal, inWorldPos, material.normalStrength, normalMap, inUV);
    vec3 V = normalize(camera.worldPos.xyz - inWorldPos);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < lightParams.lights.length(); i++) 
    {
        vec3 lightPos = lightParams.lights[i].xyz;
        vec3 L = lightPos - inWorldPos;

        // light radiance
        float distance = length(L);
        float attenuation = clamp(lightAttenuation / (1.0 + distance * distance), 0.0, 1.0);
        float lightIntensity = lightParams.lights[i].a;
        vec3 radiance = lightColor * lightIntensity * attenuation;

        // diffuse + specular BRDF
        L /= distance;
        vec3 brdf = cook_torrance_ggx(L, V, N, albedo, metallic, specular, roughness);
        // add to total outgoing radiance
        Lo += brdf * radiance * diffuse_lambert(L, N); 
    }

    float envExposure = lightParams.exposure;
    float dotNV = max(dot(N, V), 0.0);
    vec3 kS = f_schlick_roughness(dotNV, albedo, metallic, specular, roughness);
    vec3 diffuse = ibl_diffuse(N, kS, albedo, metallic, irradianceMap, envExposure);
    vec3 indirectSpecular = ibl_specular(N, V, kS, roughness, prefilteredMap, envExposure, brdfLUT);

    vec3 ambient = lightParams.ambient + diffuse + indirectSpecular;
    vec3 color = (ambient + Lo) * ao;
    color = gammaCorrection(color);

    outColor = vec4(color.rgb, 1.0);
}
