/* -------------------------------------------------------------------------------
* Copyright (c) 2021, OLogN Technologies AG
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


#ifndef SAFE_MEMORY_ALGORITHM_H
#define SAFE_MEMORY_ALGORITHM_H

#include <EASTL/internal/config.h>
#include <EASTL/algorithm.h>
#include <safememory/detail/array_iterator.h>
#include <safememory/detail/hashtable_iterator.h>


/** \file
 * \brief algorithms with optimizations for \c safememory iterators.
 * 
 * This is currently a sample of how algorithms can be optimized to work with \c safememory
 * iterators while delegating to \c eastl algorithms the actual work.
  */ 


namespace safememory {

	template <typename IT, typename T>
	IT find(IT first, IT last, const T& value) {
		return eastl::find(first, last, value);
	}


	template <typename T, bool is_const, typename ArrPtr, bool is_dezombiefy>
	detail::array_heap_safe_iterator<T, is_const, ArrPtr, is_dezombiefy>
	find(const detail::array_heap_safe_iterator<T, is_const, ArrPtr, is_dezombiefy>& first, const detail::array_heap_safe_iterator<T, is_const, ArrPtr, is_dezombiefy>& last, const T& value) {
		auto p = first.toRawOther(last);
		auto r = eastl::find(p.first, p.second, value);
		return detail::array_heap_safe_iterator<T, is_const, ArrPtr, is_dezombiefy>::makeIt(first, r);
	}

	template <typename T, bool is_const, typename ArrPtr, bool is_dezombiefy>
	detail::array_stack_only_iterator<T, is_const, ArrPtr, is_dezombiefy>
	find(const detail::array_stack_only_iterator<T, is_const, ArrPtr, is_dezombiefy>& first, const detail::array_stack_only_iterator<T, is_const, ArrPtr, is_dezombiefy>& last, const T& value) {
		auto p = first.toRawOther(last);
		auto r = eastl::find(p.first, p.second, value);
		return detail::array_stack_only_iterator<T, is_const, ArrPtr, is_dezombiefy>::makeIt(first, r);
	}

	/**
	 * \c hashtable_stack_only_iterator can't be optimized, since we can easily check
	 * the iterator pair is a valid range.
	 * 
	 * but \c hashtable_heap_safe_iterator can be optimized to \c hashtable_stack_only_iterator
	 * 
	 */
	template <typename B1, typename B2, typename A, typename T>
	detail::hashtable_heap_safe_iterator<B1, B2, A>
	find(const detail::hashtable_heap_safe_iterator<B1, B2, A>& first, const detail::hashtable_heap_safe_iterator<B1, B2, A>& last, const T& value) {
	
		using stack_only = detail::hashtable_stack_only_iterator<B1, B2, A>;
		
		auto r = eastl::find(stack_only::fromBase(first.toBase()), stack_only::fromBase(last.toBase()), value);
		return detail::hashtable_heap_safe_iterator<B1, B2, A>::makeIt(r.toBase(), first);
	}

} // namespace safememory


#endif // Header include guard

