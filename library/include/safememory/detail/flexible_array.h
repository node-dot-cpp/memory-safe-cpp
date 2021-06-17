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

#ifndef SAFE_MEMORY_DETAIL_FLEXIBLE_ARRAY_H
#define SAFE_MEMORY_DETAIL_FLEXIBLE_ARRAY_H

#include <initializer_list>
#include <EASTL/internal/config.h> // for eastl_size_t

namespace safememory::detail {

/** 
 * \brief Flexible array class for allocation of arrays.
 * 
 * Very similar to a C 'flexible array member' https://en.wikipedia.org/wiki/Flexible_array_member
 * 
 * While this class has the concept of an array or buffer,
 * it doesn't actually have the memory of the array.
 * Nor it will construct or destruct any of its elements.
 * It is assumed that the allocator will reserve enought memory right after this class
 * to actually to put the array elements.
 * This class is coupled with \c allocate_array function.
 * 
 */ 
template<class T>
struct flexible_array
{
	typedef flexible_array<T> this_type;
	typedef eastl_size_t       size_type;

	size_type sz = 0;
	alignas(T) char _begin;
	
public:
	flexible_array(size_type count) :sz(count) {}

	flexible_array(const flexible_array&) = delete;
	flexible_array(flexible_array&&) = delete;

	flexible_array& operator=(const flexible_array&) = delete;
	flexible_array& operator=(flexible_array&&) = delete;

	// make trivially destructible, as we don't use ownership semantics
	~flexible_array() = default;

	constexpr bool empty() const noexcept { return sz == 0; }
	constexpr size_type size() const noexcept { return sz; }
	constexpr size_type max_size() const noexcept { return sz; }

	constexpr T* data() noexcept { return reinterpret_cast<T*>(&_begin); }
	constexpr const T* data() const noexcept { return reinterpret_cast<const T*>(&_begin); }

	// we use 'eastl_size_t' for number of elements and std::size_t for actual memory size 
	static std::size_t calculateSize(size_type count) {
		// TODO calculated size is slightly bigger than actually needed, maybe fine tune
		// TODO also must check that we are not overflowing the size_t
		return sizeof(this_type) + (sizeof(T) * count);
	}
};


/**
 * 
 * Implementation of \c flexible_array that reserves enought
 * memory after the array header to a actually place elements.
 * Can be used on the stack or embedded in other ojects.
 * It still won't construct or destruct any of their elements.
 */
template<eastl_size_t SZ, class T>
struct flexible_array_with_memory : public flexible_array<T>
{
	/// we never access this array, is only here to reserve enought memory
	char buff[sizeof(T) * SZ];

public:
	flexible_array_with_memory(std::initializer_list<T> init) :flexible_array<T>(SZ) {
		auto jt = flexible_array<T>::data();
		auto it = init.begin();
		while(it != init.end()) {
			*jt = *it;
			++it;
			++jt;
		}
	}

	flexible_array_with_memory(const flexible_array_with_memory&) = delete;
	flexible_array_with_memory(flexible_array_with_memory&&) = delete;

	flexible_array_with_memory& operator=(const flexible_array_with_memory&) = delete;
	flexible_array_with_memory& operator=(flexible_array_with_memory&&) = delete;

	// ~flexible_array_with_memory() {}
};

} // namespace safememory::detail 

#endif // SAFE_MEMORY_DETAIL_FLEXIBLE_ARRAY_H
