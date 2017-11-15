#pragma once

namespace util
{
	template<class Container>
	inline size_t index_of(Container& c, typename Container::iterator it)
	{
		return std::distance(c.begin(), it);
	}

	template<class Container>
	inline size_t index_of(const Container& c, typename Container::const_iterator it)
	{
		return std::distance(c.begin(), it);
	}

	template<typename ArrayType, size_t ArraySize>
	inline size_t index_of(ArrayType(&c)[ArraySize], ArrayType* it)
	{
		size_t index = std::distance(c, it);
		assert(index<ArraySize);
		return std::distance(c, it);
	}
}
