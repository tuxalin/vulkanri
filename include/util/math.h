#pragma once

#include <algorithm>

namespace math
{
template <typename T>
T clamp(T val, T min_val, T max_val)
{
    return std::min(std::max(val, min_val), max_val);
}
}
