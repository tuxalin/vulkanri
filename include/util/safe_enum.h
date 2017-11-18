
#pragma once

#include "safe_enum_internal.h"

#include <cassert>
#include <sstream>
#include <vector>

#ifdef SAFE_ENUM_USE_CPP11
#include <functional>
#include <unordered_map>
#else
#include <tr1/functional>
#include <tr1/unordered_map>
#endif

namespace util
{
/**
 * @brief Type safe enum idiom.
 * @details Improve type-safety of native enum data type in C++03 to provide C++11 like enum classes.
 * Also extends enum with automatic string conversion, iterators(also for non-contigous enumerations), dynamic asserts
 * and enum count.
 *
 * Macro variant offers extended featurs like: string conversion, iterators, enumeration count.
 * Usage:
 * SAFE_ENUM_DECLARE(Shape,
 *                   eCircle, eSquare, eTriangle);
 *
 * You can also declare an enum class using unsigned char:
 * SAFE_ENUM_TYPE_DECLARE(Shape, unsigned char,
 *                        eCircle, eSquare, eTriangle);
 *
 * Or you can declare it manually, however extended features will not be available(eg. string conversion, iterators,
 * count): struct Color_def { enum type { eRed, eGreen, eBlue };
 * };
 * typedef util::SafeEnum<Color_def> Color;
 *
 * As before you can also declare it using unsigned char:
 * struct Shape_def {
 * enum type { eCircle, eSquare, eTriangle };
 * };
 * typedef util::SafeEnum<Shape_def, unsigned char> Shape;
 *
 * @note Macro version provides built-in iterator and to string conversion support.
 * @note To get the compile time count of the enum use Count member, eg: shape::Count.
 * @note For manual enums to have extended features you must use the SAFE_ENUM_DEFINE_FORMAT in the class declaration:
 * struct Shape_def {
 * SAFE_ENUM_DEFINE_FORMAT(eCircle, eSquare, eTriangle);
 * ...
 */
template <typename def, typename enum_type = typename def::type>
class SafeEnum : public def
{
public:
    typedef enum_type                type;
    typedef SafeEnum<def, enum_type> this_t;

    struct iterator : public std::vector<type>::const_iterator
    {
        typedef typename std::vector<type>::const_iterator base;

        iterator(const base& it);

        SafeEnum        operator*() const;
        const SafeEnum* operator->() const;
        bool            operator==(type other) const;
        bool            operator!=(type other) const;
        bool            operator==(iterator other) const;
        bool            operator!=(iterator other) const;
    };
    typedef iterator iterator_t;

    SafeEnum();
    SafeEnum(type v);
    template <typename T>
    explicit SafeEnum(T v);

    iterator        begin() const;
    static iterator end();

    static iterator first();
    static iterator last();

    type get() const;
#ifdef SAFE_ENUM_USE_CPP11
    template <typename CastType>
    explicit operator CastType() const;
#endif
    template <typename T>
    void set(T v);

    /// The number defining a value's position in a series, such as “first,” “second,” or “third.”
    size_t             ordinal() const;
    const std::string& str() const;

    template <typename T>
    SafeEnum& operator&=(T bitmask);
    template <typename T>
    SafeEnum& operator|=(T bitmask);
    SafeEnum& operator&=(SafeEnum bitmask);
    SafeEnum& operator|=(SafeEnum bitmask);
    template <typename T>
    bool operator==(T val) const;

    template <typename T>
    static SafeEnum from(T v);
    /// @note Doesn't asserts if the value is incorrect.
    template <typename T>
    static SafeEnum fromUnsafe(T v);

    static const std::string& toString(const SafeEnum& val);
    static const std::string& toString(type val);

    friend bool operator==(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ == rhs.val_;
    }
    friend bool operator!=(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ != rhs.val_;
    }
    friend bool operator<(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ < rhs.val_;
    }
    friend bool operator<=(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ <= rhs.val_;
    }
    friend bool operator>(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ > rhs.val_;
    }
    friend bool operator>=(const SafeEnum& lhs, const SafeEnum& rhs)
    {
        return lhs.val_ >= rhs.val_;
    }

private:
    type val_;
};

namespace safe_enum
{
    /**
     * @brief You can specialize this, like std::hasher, to define your own string conversion method.
     */
    template <typename T, typename EnableIfDummyType = void>
    struct stringify;

    /**
     * Default automatic conversion to string using internal parsed format.
     */
    template <typename T, typename enum_type>
    struct stringify<SafeEnum<T, enum_type> >
    {
        typedef SafeEnum<T, enum_type> type;
        const std::string&             operator()(const type& v)
        {
            return type::format().findString(v.get());
        }
    };
}

#define ENUM_TEMPLATE template <typename def, typename enum_type>
#define ENUM_QUAL SafeEnum<def, enum_type>

ENUM_TEMPLATE
ENUM_QUAL::SafeEnum()
    : val_()
{
}

ENUM_TEMPLATE
ENUM_QUAL::SafeEnum(type v)
    : val_(v)
{
}

ENUM_TEMPLATE
template <typename T>
ENUM_QUAL::SafeEnum(T v)
    : val_(static_cast<type>(v))
{
    assert(def::format().validValue(v));
}

ENUM_TEMPLATE
typename ENUM_QUAL::type ENUM_QUAL::get() const
{
    return val_;
}

#ifdef SAFE_ENUM_USE_CPP11
ENUM_TEMPLATE
template <typename CastType>
ENUM_QUAL::operator CastType() const
{
    return (CastType)val_;
}
#endif

ENUM_TEMPLATE
typename ENUM_QUAL::iterator ENUM_QUAL::begin() const
{
    return def::format().findPosition(val_);
}

ENUM_TEMPLATE
typename ENUM_QUAL::iterator ENUM_QUAL::first()
{
    return def::format().order.begin();
}

ENUM_TEMPLATE
typename ENUM_QUAL::iterator ENUM_QUAL::end()
{
    return def::format().order.end();
}

ENUM_TEMPLATE
typename ENUM_QUAL::iterator ENUM_QUAL::last()
{
    return def::format().order.end();
}

ENUM_TEMPLATE
size_t ENUM_QUAL::ordinal() const
{
    return def::format().findIndex(val_);
}

ENUM_TEMPLATE
const std::string& ENUM_QUAL::str() const
{
    const std::string& ret = safe_enum::stringify<this_t>()(*this);
    return ret;
}

ENUM_TEMPLATE
template <typename T>
void ENUM_QUAL::set(T v)
{
    assert(def::format().validValue(v));
    val_ = static_cast<type>(v);
}

ENUM_TEMPLATE
template <typename T>
ENUM_QUAL& ENUM_QUAL::operator&=(T bitmask)
{
    assert(def::format().validValue(val_ & bitmask));
    val_ = static_cast<type>(val_ & bitmask);
    return *this;
}

ENUM_TEMPLATE
template <typename T>
ENUM_QUAL& ENUM_QUAL::operator|=(T bitmask)
{
    assert(def::format().validValue(bitmask));
    val_ = static_cast<type>(val_ | bitmask);
    return *this;
}

ENUM_TEMPLATE
ENUM_QUAL& ENUM_QUAL::operator&=(SafeEnum bitmask)
{
    assert(def::format().validValue(val_ & bitmask.get()));
    val_ = static_cast<type>(val_ & bitmask.get());
    return *this;
}

ENUM_TEMPLATE
ENUM_QUAL& ENUM_QUAL::operator|=(SafeEnum bitmask)
{
    assert(def::format().validValue(bitmask.get()));
    val_ = static_cast<type>(val_ | bitmask.get());
    return *this;
}

ENUM_TEMPLATE
template <typename T>
bool ENUM_QUAL::operator==(T val) const
{
    return val_ == val;
}

ENUM_TEMPLATE
template <typename T>
ENUM_QUAL ENUM_QUAL::from(T v)
{
    assert(def::format().validValue(v));
    return SafeEnum(static_cast<type>(v));
}

ENUM_TEMPLATE
template <typename T>
ENUM_QUAL ENUM_QUAL::fromUnsafe(T v)
{
    return SafeEnum(static_cast<type>(v));
}

ENUM_TEMPLATE
const std::string& ENUM_QUAL::toString(const SafeEnum& val)
{
    const std::string& ret = val.str();
    return ret;
}

ENUM_TEMPLATE
const std::string& ENUM_QUAL::toString(type val)
{
    const std::string& ret = toString(fromType(val));
    return ret;
}

ENUM_TEMPLATE
ENUM_QUAL::iterator::iterator(const base& it)
    : base(it)
{
}

ENUM_TEMPLATE
ENUM_QUAL ENUM_QUAL::iterator::operator*() const
{
    const type val = base::operator*();
    return SafeEnum::from(val);
}

ENUM_TEMPLATE
const ENUM_QUAL* ENUM_QUAL::iterator::operator->() const
{
    const SafeEnum* val = reinterpret_cast<const SafeEnum*>(&base::operator*());
    return val;
}

ENUM_TEMPLATE
bool ENUM_QUAL::iterator::operator==(type other) const
{
    const iterator it = *this;
    return *it == other;
}

ENUM_TEMPLATE
bool ENUM_QUAL::iterator::operator!=(type other) const
{
    return !this->operator==(other);
}

ENUM_TEMPLATE
bool ENUM_QUAL::iterator::operator==(iterator other) const
{
    base it = *this;
    return it == other;
}

ENUM_TEMPLATE
bool ENUM_QUAL::iterator::operator!=(iterator other) const
{
    return !this->operator==(other);
}

}  // namespace tn

namespace std
{
#ifndef SAFE_ENUM_USE_CPP11
namespace tr1
{
#endif
    template <typename def>
    struct hash<util::SafeEnum<def> >
    {
        size_t operator()(const util::SafeEnum<def>& k) const
        {
            return hash<typename util::SafeEnum<def>::type>()(k.get());
        }
    };
#ifndef SAFE_ENUM_USE_CPP11
}  // namespace tr1
#endif
}  // namespace std

// internals

#define SAFE_ENUM_DEFINE_FORMAT(...)                                                         \
    enum type                                                                                \
    {                                                                                        \
        __VA_ARGS__,                                                                         \
        Count = _SAFE_ENUM_COUNT_VARARGS(__VA_ARGS__)                                        \
    };                                                                                       \
                                                                                             \
    static const util::safe_enum::detail::ParsedFormat<type>& format()                       \
    {                                                                                        \
        static const util::safe_enum::detail::ParsedFormat<type> __PARSED_FORMAT__ = init(); \
        return __PARSED_FORMAT__;                                                            \
    }                                                                                        \
    static util::safe_enum::detail::ParsedFormat<type> init()                                \
    {                                                                                        \
        util::safe_enum::detail::ParsedFormat<type> __PARSED_FORMAT__;                       \
        int                                         ___LAST_VALUE___ = -1;                   \
        _SAFE_ENUM_IMPL_FORMAT(__VA_ARGS__);                                                 \
        return __PARSED_FORMAT__;                                                            \
    }

#define SAFE_ENUM_DECLARE(enumName, ...)      \
    struct enumName##_                        \
    {                                         \
        SAFE_ENUM_DEFINE_FORMAT(__VA_ARGS__); \
    };                                        \
    typedef util::SafeEnum<enumName##_> enumName

#define SAFE_ENUM_TYPE_DECLARE(enumName, enumType, ...) \
    struct enumName##_                                  \
    {                                                   \
        SAFE_ENUM_DEFINE_FORMAT(__VA_ARGS__);           \
    };                                                  \
    typedef util::SafeEnum<enumName##_, enumType> enumName

namespace util
{
namespace safe_enum
{
    namespace detail
    {
        /**
         * @brief A parsed format of the enum type that holds the order, string and index.
         */
        template <typename T>
        struct ParsedFormat
        {
#ifdef SAFE_ENUM_USE_CPP11
            typedef std::unordered_map<T, std::string> StringMap;
            typedef std::unordered_map<T, size_t>      IntMap;
#else
            struct KeyHasher
            {
                std::size_t operator()(const T k) const
                {
                    return std::tr1::hash<size_t>()(k);
                }
            };
            typedef std::tr1::unordered_map<T, std::string, KeyHasher> StringMap;
            typedef std::tr1::unordered_map<T, size_t, KeyHasher>      IntMap;
#endif  //  SAFE_ENUM_USE_CPP11

            StringMap      strings;
            IntMap         position;
            std::vector<T> order;

            size_t findIndex(T val) const
            {
                typename IntMap::const_iterator it = position.find(val);
                assert(it != position.end());
                return it->second;
            }

            typename std::vector<T>::const_iterator findPosition(T val) const
            {
                return order.begin() + findIndex(val);
            }

            template <typename ValType>
            const std::string& findString(ValType val) const
            {
                const T                            mapVal = static_cast<T>(val);
                typename StringMap::const_iterator it     = strings.find(mapVal);
                assert(it != strings.end());
                return it->second;
            }

            template <typename OtherType>
            bool validValue(OtherType v) const
            {
                return position.find(static_cast<T>(v)) != position.end();
            }
        };

        inline std::string parseEnumValue(const char* args)
        {
            // setup map using their actual values
            std::string       buf1;
            std::stringstream localStream(args);
            localStream >> buf1;
            return buf1;
        }

        template <typename T>
        void addValue(detail::ParsedFormat<T>& format, detail::init_test& tv, int& lastValue, const char* enumExprStr)
        {
            typedef T type;
            if (tv.init)
            {
                lastValue = tv;
            }
            else
            {
                tv = ++lastValue;
            }
            type enumVal = static_cast<type>(tv.operator int());
            format.order.push_back(enumVal);
            format.position[enumVal] = format.order.size() - 1;
            format.strings[enumVal]  = parseEnumValue(enumExprStr);
        }
    }  // namespace detail
}  // namespace safe_enum
}  // namespace util

#undef ENUM_TEMPLATE
#undef ENUM_QUAL
#undef SAFE_ENUM_USE_CPP11
