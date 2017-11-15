#pragma once

#include <cassert>

#define assert_if(guard, cond) if(guard) assert(cond)

namespace util
{
	/**
	* @brief TypeDisplayer
	* @details Used to print type(via compiler error) for auto and template type deduction.
	*
	* @param A template argument or an auto variable.
	*
	* @example auto val = func();
	* printType(val);
	*
	* @example or in a template function/class:
	* template <typename T> void func(T val) {
	* TypeDisplayer<T> t;
	* IntDisplayer<sizeof(int16_t)> t;
	**/
	template <typename TypeDisplay>
	class TypeDisplayer;

	///@note Same as TypeDisplayer but used to print integer values.
	template <int IntValueDisplay>
	class IntDisplayer;

	template <typename T>
	void printType(const T&)
	{
		TypeDisplayer<T> t;
	}

#define UTIL_TYPE_DISYPLAYER(type) \
    template <>                  \
    class TypeDisplayer<type>    \
    {                            \
    }
}
