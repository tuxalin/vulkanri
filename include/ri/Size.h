
#pragma once

#include <ostream>

#include <util/math.h>

namespace tn
{
/// Defines the resolution size, eg. of a texture.
template <typename T = uint32_t>
struct Size
{
    typedef T type;

    Size();
    explicit Size(type size);
    Size(type width, type height);
    template <typename OtherType>
    Size(OtherType width, OtherType height);
    Size(const Size& other);

    void set(type width, type height);
    template <typename OtherType>
    Size& operator=(const Size<OtherType>& other);

    uint32_t pixelCount() const;

    bool equals(type width, type height) const;
    bool isPowerOfTwo() const;

    Size operator*(float scale) const;
    bool operator==(const Size& other) const;
    bool operator>(const Size& other) const;
    bool operator<(const Size& other) const;
    bool operator<=(const Size& other) const;
    bool operator>=(const Size& other) const;

    operator bool() const;

    type width;
    type height;
};  // struct Size

typedef Size<uint32_t> Sizei;
typedef Size<float>    Sizef;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline Size<T>::Size()
    : width(0)
    , height(0)
{
}

template <typename T>
inline Size<T>::Size(type size)
    : width(size)
    , height(size)
{
}

template <typename T>
inline Size<T>::Size(type width, type height)
    : width(width)
    , height(height)
{
}

template <typename T>
template <typename OtherType>
inline Size<T>::Size(OtherType width_, OtherType height_)
    : width((type)width_)
    , height((type)height_)
{
    assert(width_ > 0);
    assert(height_ > 0);
}

template <typename T>
inline Size<T>::Size(const Size& other)
    : width(other.width)
    , height(other.height)
{
    assert(width >= 0);
    assert(height >= 0);
}

template <typename T>
inline void Size<T>::set(type width, type height)
{
    this->width  = width;
    this->height = height;
}

template <typename T>
template <typename OtherType>
Size<T>& Size<T>::operator=(const Size<OtherType>& other)
{
    width  = other.width;
    height = other.height;
    return *this;
}

template <typename T>
inline uint32_t Size<T>::pixelCount() const
{
    return width * height;
}

template <typename T>
inline bool Size<T>::equals(type width, type height) const
{
    return this->width == width && this->height == height;
}

template <typename T>
inline bool Size<T>::isPowerOfTwo() const
{
    return tn::math::isPowerOfTwo(width) && tn::math::isPowerOfTwo(height);
}

template <typename T>
inline Size<T> Size<T>::operator*(float scale) const
{
    return Size(width * scale, height * scale);
}

template <typename T>
inline bool Size<T>::operator==(const Size& other) const
{
    return width == other.width && height == other.height;
}

template <typename T>
inline bool Size<T>::operator>(const Size& other) const
{
    return width > other.width || height > other.height;
}

template <typename T>
inline bool Size<T>::operator<(const Size& other) const
{
    return width < other.width || height < other.height;
}

template <typename T>
inline bool Size<T>::operator<=(const Size& other) const
{
    return !(*this > other);
}

template <typename T>
inline bool Size<T>::operator>=(const Size& other) const
{
    return !(*this < other);
}

template <typename T>
inline Size<T>::operator bool() const
{
    return width >= 2 && height >= 2;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const Size<T>& size)
{
    return stream << "tn::Size width= " << size.width << " height= " << size.height << "\n";
}

}  // namespace ri
