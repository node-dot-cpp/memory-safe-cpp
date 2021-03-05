/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef SAFE_MEMORY_DETAIL_ARRAY_OF
#define SAFE_MEMORY_DETAIL_ARRAY_OF

#include <utility> //for std::pair
#include <safememory/memory_safety.h>

// mb: TODO rename this file an everything inside it, as name is too similar to safememory::array

// namespace safememory {
// 	//fwd
// 	template<typename, memory_safety> class soft_ptr;
// }

namespace safememory::detail {

/** 
 * \brief Helper class for allocation of arrays.
 * 
 * While this class has the concept of an array or buffer,
 * it doesn't actually have the memory of the array.
 * Nor it will construct or destruct any of its elements.
 * It is assumed that the allocator will give enought memory right after this class
 * to actually to put the array elements.
 * This class is coupled with \c allocate_array function.
 * 
 * Very similar to a C 'flexible array member'
 */ 
template<class T>
struct array_of
{
	typedef array_of<T> this_type;
	typedef size_t      size_type;

	size_type _capacity = 0;
	alignas(T) char _begin;

public:
	array_of(size_type capacity) :_capacity(capacity) {}

	array_of(const array_of&) = delete;
	array_of(array_of&&) = delete;

	array_of& operator=(const array_of&) = delete;
	array_of& operator=(array_of&&) = delete;

	// ~array_of() {}

	constexpr bool empty() const noexcept { return _capacity == 0; }
	constexpr size_type size() const noexcept { return _capacity; }
	constexpr size_type max_size() const noexcept { return _capacity; }

	constexpr T* data() noexcept { return reinterpret_cast<T*>(&_begin); }
	constexpr const T* data() const noexcept { return reinterpret_cast<const T*>(&_begin); }

	static size_type calculateSize(size_type size) {
		// TODO here we should fine tune the sizes of array_of<T> 
		return static_cast<size_type>(sizeof(this_type) + (sizeof(T) * size));
	}
};


/**
 * 
 * Implementation of \c array_of that reserves enought
 * memory after the array header to a actually place elements.
 * Can be used on the stack or embedded in other ojects.
 * It still won't construct or destruct any of their elements.
 */
template<size_t SZ, class T>
struct fixed_array_of : public array_of<T>
{
	/// we never use this array, is only here to reserve enought memory
	char buff[sizeof(T) * SZ];

public:
	fixed_array_of(std::initializer_list<T> init) :array_of<T>(SZ) {
		auto jt = array_of<T>::data();
		auto it = init.begin();
		while(it != init.end()) {
			*jt = *it;
			++it;
			++jt;
		}
	}

	fixed_array_of(const fixed_array_of&) = delete;
	fixed_array_of(fixed_array_of&&) = delete;

	fixed_array_of& operator=(const fixed_array_of&) = delete;
	fixed_array_of& operator=(fixed_array_of&&) = delete;

	~fixed_array_of() {}
};

} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_ARRAY_OF
