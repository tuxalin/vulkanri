
#include "../../resources/shaders/math.glsl"

// Normal Distribution
float d_ggx(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom); 
}

// Geometric Shadowing
float g_schlickSmithGGX(float dotN, float k)
{
    return dotN / (dotN * (1.0 - k) + k);
}

float g_schlickSmithGGX(float dotNL, float dotNV, float roughness)
{
    float alpha = (roughness + 1.0);
    float k = (alpha * alpha) / 8.0;
    float GL = g_schlickSmithGGX(dotNL, k);
    float GV = g_schlickSmithGGX(dotNV, k);
    return GL * GV;
}

float g_ibl_schlickSmithGGX(float dotNL, float dotNV, float roughness)
{
    float alpha = roughness;
    float k = (alpha * alpha) / 2.0; // special remap of k for IBL lighting
    float GL = g_schlickSmithGGX(dotNL, k);
    float GV = g_schlickSmithGGX(dotNV, k);
    return GL * GV;
}

// Fresnel
vec3 f_schlick(float cosTheta, vec3 albedo, float metallic, float specular)
{
    vec3 F0 = mix(vec3(0.04), albedo, metallic) * specular;
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
    return F;    
}

vec3 f_schlick_roughness(float cosTheta, vec3 albedo, float metallic, float specular, float roughness)
{
    // apply roughness term in the Fresnel-Schlick equation as described by Sebastien Lagarde
    // to reduce the indirect Fresnel reflection for dielectric materials
    vec3 F0 = mix(vec3(0.04), albedo, metallic) * specular;
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

// Lambertian reflectance
vec3 d_lambert(vec3 kS, vec3 albedo, float metallic)
{
    // for energy conservation, inverse of the reflected light
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness as metals don't have diffuse lighting
    kD *= 1.0 - metallic; 
    return kD * albedo * INV_PI; // normalize surface color
}

// Diffuse + Specular BRDF
vec3 cook_torrance_ggx(vec3 L, vec3 V, vec3 N, vec3 albedo, float metallic, float specular, float roughness)
{
    // Precalculate half-vector and dot products    
    vec3 H = normalize (V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

    vec3 color = vec3(0.0);
    if (dotNL > 0.0)
    {
        // Normal distribution function
        float NDF = d_ggx(dotNH, roughness); 
        // Geometric shadowing term 
        float G = g_schlickSmithGGX(dotNL, dotNV, roughness);
        // Fresnel factor 
        vec3 F = f_schlick(dotNV, albedo, metallic, specular);
        vec3 S = NDF * F * G / (4.0 * dotNL * dotNV + 0.001); // add delta to prevent division by zero

        color = d_lambert(F, albedo, metallic) + S;
    }

    return color;
}

float diffuse_lambert(vec3 L, vec3 N) 
{
    return clamp(dot(N, L), 0.0, 1.0);
}

vec3 ibl_diffuse(vec3 N, vec3 kS, vec3 albedo, float metallic, samplerCube irradianceMap, float exposure)
{
    vec3 kD = (1.0 - kS) * (1.0 - metallic);    
    vec3 irradiance = texture(irradianceMap, N).rgb; 
    irradiance = vec3(1.0) - exp(-irradiance * exposure); // apply exposure

    vec3 diffuse = kD * albedo * irradiance;
    return diffuse;
}

vec3 ibl_specular(vec3 N, vec3 V, vec3 F, float roughness, samplerCube prefilteredMap, float exposure, sampler2D brdfLUT)
{
    const float maxReflectionLOD = 4.0;

    // sample both the pre-filter map and the BRDF lut
    vec3 R = reflect(-V, N); 
    vec3 prefilteredColor = textureLod(prefilteredMap, R,  roughness * maxReflectionLOD).rgb;    
    prefilteredColor = vec3(1.0) - exp(-prefilteredColor * exposure); // apply exposure
    float dotNV = max(dot(N, V), 0.005);
    vec2 brdf  = texture(brdfLUT, vec2(dotNV, roughness)).rg;

    // combine them together as per the Split-Sum approximation to get the IBL specular part
    vec3 indirectSpecular = prefilteredColor * (F * brdf.x + brdf.y);
    return indirectSpecular;
}
