
vec3 gammaEncode(vec3 color) 
{
    return pow(color, vec3(1.0 / 2.2));
}

vec3 gammaDecode(vec3 color) 
{
    return pow(color, vec3(2.2));
}

vec3 gammaCorrection(vec3 color) 
{
    return gammaEncode(color);
}
