#pragma once

#include <algorithm>
#include <cmath>

namespace math
{
constexpr double kPI =
    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647;
constexpr float  kPIf               = (float)kPI;
constexpr float  kDegreesToRadiansf = kPIf / 180.f;
constexpr float  kRadiansToDegreesf = 180.f / kPIf;
constexpr double kDegreesToRadians  = kPI / 180.f;
constexpr double kRadiansToDegrees  = 180.f / kPI;

constexpr float k360rad   = 360 * kDegreesToRadiansf;
constexpr float k180rad   = 180 * kDegreesToRadiansf;
constexpr float kPos90rad = 90 * kDegreesToRadiansf;
constexpr float kNeg90rad = -90 * kDegreesToRadiansf;

constexpr float  k2PIf   = 2 * kPIf;
constexpr float  kPIBy2f = kPIf / 2;
constexpr float  kPIBy4f = kPIf / 4;
constexpr double k2PI    = 2 * kPI;
constexpr double kPIBy2  = kPI / 2;
constexpr double kPIBy4  = kPI / 4;

// Fast aproximation of atan2, |error| < 0.005 radians
float fastAtan2(float y, float x);

template <typename T>
T        ln(T val);
uint32_t log2(uint32_t val);
int32_t  log2i(uint32_t val);

// The mod functon differs from '%' in that it behaves correctly when either the numerator or denominator is
// negative.
int mod(int n, int d);
// fast SQRT function used in Quake 3, based on Newton's approximation
float fastSqrt(float x);
// fast inverse SQRT function used in Quake 3, based on Newton's approximation
float fastInvSqrt(float x);

// round half way cases to infinity
float roundHalfUp(float x);
// Returns the fractional part of a number
float fract(float x);

template <typename T>
bool inRange(T val, T lowerBound, T upperBound);
template <typename T>
bool overlap(T lowerBound1, T upperBound1, T lowerBound2, T upperBound2);
template <typename T>
bool equal(T val, T expected, T epsilon = (T)1e-5);
template <typename T>
void minMax(T val1, T val2, T& minOut, T& maxOut);

// wraps the value in the [min,max] inteval
template <typename T>
T wrap(T a, T min, T max);
template <typename T>
T clamp(T val, T min_val, T max_val);
template <typename T>
T saturate(T a);
template <typename T>
T smoothStep(T lhs, T rhs, T t);
// maps x from interval [a1, a2] to [b1, b2]
template <typename T>
T mapTo(T x, T a1, T a2, T b1, T b2);
// maps a simple value in the [a,b] range to the [0,1] interval
template <typename T>
double parameterize(T value, T start, T end);
// linear interpolation between a, b given t
template <typename T>
T lerp(T a, T b, float t);
// linearly interpolates the angle via the shortest route around the circle.
template <typename T>
T lerpDeg(T a, T b, float t);

// greatest common divisor between two numbers
int gcd(int a, int b);
// return the closest power of two size for a given number
int  closestPowerOfTwo(int value);
bool isPowerOfTwo(unsigned int x);

/// Convert float to half float. From https://gist.github.com/martinkallman/5049614
inline unsigned short toHalfFloat(float value);
/// Convert half float to float.
inline float fromHalfFloat(unsigned short value);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float fastAtan2(float y, float x)
{
    if (x == 0.0)
    {
        if (y > 0.0)
            return math::kPIBy2f;
        if (y == 0.0)
            return 0.0;
        return -math::kPIBy2f;
    }
    float       res;
    const float z = y / x;
    if (fabs(z) < 1.0)
    {
        res = z / (1.0f + 0.28f * z * z);
        if (x < 0.f)
        {
            if (y < 0.f)
                return res - math::kPIf;
            return res + math::kPIf;
        }
    }
    else
    {
        res = math::kPIBy2f - z / (z * z + 0.28f);
        if (y < 0.0)
            return res - math::kPIf;
    }
    return res;
}

template <typename T>
T ln(T val)
{
    return std::log(val);
}

inline int log2i(uint32_t n)
{
#define S(k)                     \
    if (n >= (UINT32_C(1) << k)) \
    {                            \
        i += k;                  \
        n >>= k;                 \
    }
    int i = -(n == 0);
    S(16);
    S(8);
    S(4);
    S(2);
    S(1);
    return i;
#undef S
}

inline uint32_t log2(uint32_t val)
{
    int32_t res = log2i(val);
    assert(res >= 0);
    return (uint32_t)res;
}

inline int mod(int n, int d)
{
    int m = n % d;
    return (((m < 0) && (d > 0)) || ((m > 0) && (d < 0))) ? (m + d) : m;
}

inline float fastSqrt(float x)
{
    const float xhalf = 0.5f * x;
    int         i     = *(int*)&x;
    i                 = 0x5f3759df - (i >> 1);
    x                 = *(float*)&i;
    x                 = x * (1.5f - xhalf * x * x);

    if (x == 0.0f)
    {
        if (xhalf >= 0.0f)
            return sqrtf(xhalf * 2.0f);
        return 0.0f;
    }

    return 1.0f / x;
}

inline float fastInvSqrt(float x)
{
    float xhalf = 0.5f * x;
    int   i     = *(int*)&x;                   // get bits for floating value
    i           = 0x5f375a86 - (i >> 1);       // gives initial guess y0
    x           = *(float*)&i;                 // convert bits back to float
    x           = x * (1.5f - xhalf * x * x);  // Newton step, repeating increases accuracy
    assert(std::isnormal(x));
    return x;
}

inline float roundHalfUp(float x)
{
    return floorf(x + 0.5f);
}

inline double roundHalfUp(double x)
{
    return floor(x + 0.5);
}

inline float fract(float x)
{
    return x - int(x);
}

inline double fract(double x)
{
    return x - long(x);
}

template <typename T>
inline bool inRange(T val, T lowerBound, T upperBound)
{
    return lowerBound <= val && val <= upperBound;
}

template <typename T>
inline bool overlap(T lowerBound1, T upperBound1, T lowerBound2, T upperBound2)
{
    return inRange(lowerBound2, lowerBound1, upperBound1) || inRange(lowerBound1, lowerBound2, upperBound2);
}

template <typename T>
inline bool equal(T val, T expected, T epsilon /*= 1e-5*/)
{
    assert(epsilon >= 0);
    return tn::abs(val - expected) <= epsilon;
}

template <typename T>
inline void minMax(T val1, T val2, T& minOut, T& maxOut)
{
    if (val1 < val2)
    {
        minOut = val1;
        maxOut = val2;
    }
    else
    {
        minOut = val2;
        maxOut = val1;
    }
}

template <typename T>
inline T wrap(T a, T min, T max)
{
    assert(max > min);

    const T d = max - min;
    const T s = a - min;
    const T q = s / d;
    const T m = q - tn::floor(q);
    return m * d + min;
}

template <>
inline int32_t wrap(int32_t a, int32_t min, int32_t max)
{
    const int32_t d = max - min;
    const int32_t s = a - min;
    const int32_t r = mod(s, d);
    return r + min;
}

template <typename T>
T clamp(T val, T min_val, T max_val)
{
    return std::min(std::max(val, min_val), max_val);
}

template <typename T>
T saturate(T a)
{
    return clamp(a, (T)0, (T)1);
}

template <typename T>
inline T smoothStep(T lhs, T rhs, T t)
{
    t = saturate((t - lhs) / (rhs - lhs));
    return t * t * (3.0 - 2.0 * t);
}

template <typename T>
T mapTo(T x, T a1, T a2, T b1, T b2)
{
    return (x - a1) * (b2 - b1) / (a2 - a1) + b1;
}

template <typename T>
double parameterize(T value, T start, T end)
{
    assert(start != end);
    if (start == end)
        return 0.;
    double retValue = double(value - start) / (end - start);
    return retValue;
}

template <typename T>
inline T lerp(T a, T b, float t)
{
    return a + ((b - a) * t);
}

template <typename T>
T lerpDeg(T a, T b, float t)
{
    a = wrap(a, T(-180), T(180));
    b = wrap(b, T(-180), T(180));

    const T diff = (b - a);
    if (diff >= 0)
        return (diff < T(180)) ? lerp(a, b, t) : wrap(lerp(a, b - T(360), t), T(-180), T(180));
    else
        return ((a - b) < T(180)) ? lerp(a, b, t) : wrap(lerp(a, b + T(360), t), T(-180), T(180));
}

inline int gcd(int a, int b)
{
    int c = a % b;
    while (c != 0)
    {
        a = b;
        b = c;
        c = a % b;
    }

    return (b);
}

inline int closestPowerOfTwo(int value)
{
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --value;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return ++value;
}

inline int closestPowerOfTwo(float value)
{
    assert(value > 1);
    return 1 << static_cast<int>(ceilf(log2f(value)));
}

inline bool isPowerOfTwo(unsigned int x)
{
    return x && !(x & (x - 1));
}

/// Convert float to half float. From https://gist.github.com/martinkallman/5049614
inline unsigned short toHalfFloat(float value)
{
    unsigned inu = *((unsigned*)&value);
    unsigned t1  = inu & 0x7fffffff;  // Non-sign bits
    unsigned t2  = inu & 0x80000000;  // Sign bit
    unsigned t3  = inu & 0x7f800000;  // Exponent

    t1 >>= 13;  // Align mantissa on MSB
    t2 >>= 16;  // Shift sign bit into position

    t1 -= 0x1c000;  // Adjust bias

    t1 = (t3 < 0x38800000) ? 0 : t1;       // Flush-to-zero
    t1 = (t3 > 0x47000000) ? 0x7bff : t1;  // Clamp-to-max
    t1 = (t3 == 0 ? 0 : t1);               // Denormals-as-zero

    t1 |= t2;  // Re-insert sign bit

    return (unsigned short)t1;
}

/// Convert half float to float. From https://gist.github.com/martinkallman/5049614
inline float fromHalfFloat(unsigned short value)
{
    unsigned t1 = value & 0x7fff;  // Non-sign bits
    unsigned t2 = value & 0x8000;  // Sign bit
    unsigned t3 = value & 0x7c00;  // Exponent

    t1 <<= 13;  // Align mantissa on MSB
    t2 <<= 16;  // Shift sign bit into position

    t1 += 0x38000000;  // Adjust bias

    t1 = (t3 == 0 ? 0 : t1);  // Denormals-as-zero

    t1 |= t2;  // Re-insert sign bit

    float out;
    *((unsigned*)&out) = t1;
    return out;
}
}
