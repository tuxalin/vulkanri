

vec3 unpackNormal(vec4 n) 
{
    n.xyz = n.xyz * 2.0 - 1.0;
    return n.xyz;
}

vec3 unpackNormalRecZ(vec4 packedNormal) 
{
	vec3 normal;
    normal.xy = packedNormal.wy * 2.0 - 1.0;
    normal.z = sqrt(1 - normal.x*normal.x - normal.y * normal.y);
    return normal;
}

// based on Derivative Maps by Rory Driscoll

mat3 computeTangentFrame(vec3 normal, vec3 position, vec2 uv)
{
	vec3 dp1 = dFdx(position);
	vec3 dp2 = dFdy(position);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);

	mat3 M = mat3(dp1, dp2, cross(dp1, dp2));
	mat2x3 inverseM;
	inverseM[0] = vec3(cross(M[1], M[2]));
	inverseM[1] = vec3(cross(M[2], M[0]));

	vec3 T = normalize(inverseM * vec2(duv1.x, duv2.x));
	vec3 B = cross(T, normal);
	return mat3(T, B, normal);
}

// Returns the surface normal using screen-space partial derivatives of the uv and position coordinates.
vec3 computeSurfaceNormal(vec3 normal, vec3 position, sampler2D tex, vec2 uv)
{
	mat3 tangentFrame = computeTangentFrame(normal, position, uv);

#ifndef USE_FILTERING
	normal = unpackNormal(texture(tex, uv));
#else
	vec2 duv1 = dFdx(uv) * 2;
	vec2 duv2 = dFdy(uv) * 2;
	normal = unpackNormal(textureGrad(tex, uv, duv1, duv2));
#endif
	return normalize(normal * tangentFrame);
}