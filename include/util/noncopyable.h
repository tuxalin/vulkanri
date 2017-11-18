#pragma once

#if defined(_MSC_VER)
#if _MSC_VER >= 1800
#define NON_COPYABLE_USE_CPP11
#endif
#elif __cplusplus >= 201103L
#define NON_COPYABLE_USE_CPP11
#endif

namespace util
{
class noncopyable
{
#ifdef NON_COPYABLE_USE_CPP11
protected:
    noncopyable() = default;

private:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
#else
    noncopyable(const noncopyable&);
    noncopyable& operator=(const noncopyable&);
#endif
};
}

#undef NON_COPYABLE_USE_CPP11
