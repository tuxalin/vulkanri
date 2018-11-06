
const float PI = 3.14159265359;

// Normal Distribution
float d_ggx(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
	return (alpha2)/(PI * denom*denom); 
}

// Geometric Shadowing
float g_schlicksmithGGX(float dotNL, float dotNV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;
	float GL = dotNL / (dotNL * (1.0 - k) + k);
	float GV = dotNV / (dotNV * (1.0 - k) + k);
	return GL * GV;
}

// Fresnel
vec3 f_schlick(float cosTheta, vec3 albedo, float metallic, float specular)
{
	vec3 F0 = mix(vec3(0.04), albedo, metallic) * specular;
	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
	return F;    
}

// Lambertian reflectance
vec3 D_Lambert(vec3 kS, vec3 albedo, float metallic)
{
	// for energy conservation, inverse of the reflected light
	vec3 kD = vec3(1.0) - kS;
	// multiply kD by the inverse metalness as metals don't have diffuse lighting
	kD *= 1.0 - metallic; 
	return kD * albedo / PI; // normalize surface color
}

// Diffuse + Specular BRDF
vec3 cookTorrance(vec3 L, vec3 V, vec3 N, vec3 albedo, float metallic, float specular, float roughness)
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
		float G = g_schlicksmithGGX(dotNL, dotNV, roughness);
		// Fresnel factor 
		vec3 F = f_schlick(dotNV, albedo, metallic, specular);

		vec3 s = NDF * F * G / (4.0 * dotNL * dotNV + 0.001); // add 0.001 to prevent divide by zero

		color = D_Lambert(F, albedo, metallic) + s;
	}

	return color;
}

float diffuseLambert(vec3 L, vec3 N) 
{
	return clamp(dot(N, L), 0.0, 1.0);
}

vec3 gammaEncode(vec3 color) 
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 gammaDecode(vec3 color) 
{
    return pow(color, vec3(2.2));
}