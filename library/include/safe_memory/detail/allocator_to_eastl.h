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

#ifndef SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H
#define SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H

#include <safe_memory/safe_ptr.h>
#include <safe_memory/detail/soft_ptr_with_zero_offset.h>
#include <safe_memory/detail/array_of.h>
#include <EASTL/allocator.h>


namespace eastl {
extern void* gpEmptyBucketArray[2];
}

namespace safe_memory::detail {

extern fixed_array_of<2, soft_ptr_with_zero_offset_base> gpSafeMemoryEmptyBucketArray;

using nodecpp::safememory::memory_safety;
using nodecpp::safememory::FirstControlBlock;
using nodecpp::safememory::module_id;
using nodecpp::safememory::getPrefixByteCount;
using nodecpp::safememory::zombieAllocate;
using nodecpp::safememory::getControlBlock_;
using nodecpp::safememory::zombieDeallocate;
using nodecpp::safememory::getAllocatedBlock_;
using nodecpp::safememory::allocate;
using nodecpp::safememory::deallocate;
using nodecpp::safememory::soft_ptr_impl;
using nodecpp::safememory::fbc_ptr_t;



template<class T, bool zeroed = false>
soft_ptr_with_zero_offset_impl<T> allocate_impl() {

	std::size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();

	std::size_t total = head + sizeof(T);
	void* data = zombieAllocate(total);

	if constexpr (zeroed)
		std::memset(data, 0, total);

	T* dataForObj = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(data) + head);
	
	auto cb = getControlBlock_(dataForObj);
	cb->init();

	return {make_zero_offset_t(), dataForObj};
}

template<class T, bool zeroed = !std::is_trivial<T>::value>
soft_ptr_with_zero_offset_impl<array_of<T>> allocate_array_impl(std::size_t count) {

	std::size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();

	// TODO here we should fine tune the sizes of array_of2<T> 
	std::size_t total = head + array_of<T>::calculateSize(count);
	void* data = zombieAllocate(total);

	// non trivial types get zeroed memory, just in case we get to deref
	// a non initialized position
	if constexpr (zeroed)
		std::memset(data, 0, total);

	array_of<T>* dataForObj = reinterpret_cast<array_of<T>*>(reinterpret_cast<uintptr_t>(data) + head);
	
	auto cb = getControlBlock_(dataForObj);
	cb->init();

	::new ( dataForObj ) array_of<T>(count);

	return {make_zero_offset_t(), dataForObj};
}

template<class T>
void deallocate_impl(soft_ptr_with_zero_offset_impl<T>& p) {

	if (p) {
		T* dataForObj = p.get_raw_ptr();
//		dataForObj->~T(); //we don't destruct here
		auto cb = getControlBlock_(dataForObj);
		cb->template updatePtrForListItemsWithInvalidPtr<T>();
		zombieDeallocate( getAllocatedBlock_(dataForObj) );
		cb->clear();
	}
}

template<class T>
void deallocate_array_impl(soft_ptr_with_zero_offset_impl<array_of<T>>& p) {

	if (p) {
		array_of<T>* dataForObj = p.get_array_of_ptr();
		dataForObj->~array_of<T>();
		auto cb = getControlBlock_(dataForObj);
		cb->template updatePtrForListItemsWithInvalidPtr<array_of<T>>();
		zombieDeallocate( getAllocatedBlock_(dataForObj) );
		cb->clear();
	}
}


template<class T>
T* allocate_raw(std::size_t count) {

	return reinterpret_cast<T*>(allocate(sizeof(T) * count));
}

template<class T>
void deallocate_raw(T* p) {

	if (p)
		deallocate(p);
}


template<class T>
soft_ptr_with_zero_offset_no_checks<T> allocate_no_checks() {

	std::size_t sz = sizeof(T);
	T* dataForObj = reinterpret_cast<T*>(allocate(sz));
	
	return {make_zero_offset_t(), dataForObj};
}

template<class T>
soft_ptr_with_zero_offset_no_checks<array_of<T>> allocate_array_no_checks(std::size_t count) {
	// TODO here we should fine tune the sizes of array_of2<T> 
	std::size_t sz = array_of<T>::calculateSize(count);
	array_of<T>* dataForObj = reinterpret_cast<array_of<T>*>(allocate(sz));
	
	::new ( dataForObj ) array_of<T>(count);
	return {make_zero_offset_t(), dataForObj};
}

template<class T>
void deallocate_no_checks(soft_ptr_with_zero_offset_no_checks<T>& p) {

	if (p) {
		T* dataForObj = p.get_raw_ptr();
		//dataForObj->~T(); //don't destruct here
		deallocate(dataForObj);
	}
}

template<class T>
void deallocate_array_no_checks(soft_ptr_with_zero_offset_no_checks<array_of<T>>& p) {

	if (p) {
		array_of<T>* dataForObj = p.get_array_of_ptr();
		dataForObj->~array_of<T>();
		deallocate(dataForObj);
	}
}


template<typename T, memory_safety Safety>
using array_of_iterator_heap = std::conditional_t<Safety == memory_safety::safe,
			array_of_iterator<T, false, soft_ptr_impl<array_of<T>>>,
			array_of_iterator<T, false, soft_ptr_no_checks<array_of<T>>>
			>;

template<typename T, memory_safety Safety>
using const_array_of_iterator_heap = std::conditional_t<Safety == memory_safety::safe,
			array_of_iterator<T, true, soft_ptr_impl<array_of<T>>>,
			array_of_iterator<T, true, soft_ptr_no_checks<array_of<T>>>
			>;

// two special values used inside eastl::hasttable
// soft_ptr needs to have special behaviour around this values.

template<class T>
constexpr T* hashtable_sentinel() {
	return reinterpret_cast<T*>((uintptr_t)~0);
}

template<class T>
constexpr array_of<T>* empty_hashtable() {
	return reinterpret_cast<array_of<T>*>(&gpSafeMemoryEmptyBucketArray);
}


template<class T>
constexpr T* empty_hashtable_raw() {
	return reinterpret_cast<T*>(&eastl::gpEmptyBucketArray);
}


class base_allocator_to_eastl_impl {
public:

	static constexpr memory_safety is_safe = memory_safety::safe;
	static constexpr bool use_base_iterator = false;

	template<class T>
	using pointer = soft_ptr_with_zero_offset_impl<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_impl<array_of<T>>;

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0) {
		return allocate_array_impl<T>(count);
	}

	template<class T>
	array_pointer<T> allocate_array_zeroed(std::size_t count, int flags = 0) {
		return allocate_array_impl<T, true>(count);
	}

	template<class T>
	void deallocate_array(array_pointer<T>& p, std::size_t count) {
		deallocate_array_impl(p);
	}

	template<class T>
	pointer<T> allocate_node() {
		return allocate_impl<T>();
	}

	template<class T>
	void deallocate_node(pointer<T>& p) {
		deallocate_impl(p);
	}

	template<class T>
	static T* to_raw(const pointer<T>& p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static T* to_raw(const array_pointer<T>& p) {
		return p.get_raw_begin();
	}

	//stateless
	bool operator==(const base_allocator_to_eastl_impl&) const { return true; }
	bool operator!=(const base_allocator_to_eastl_impl&) const { return false; }
};

class base_allocator_to_eastl_raw {
public:

	static constexpr memory_safety is_safe = memory_safety::none; 
	static constexpr bool use_base_iterator = true;

	template<class T>
	using pointer = T*;

	template<class T>
	using array_pointer = T*;

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0) {
		return allocate_raw<T>(count);
	}

	template<class T>
	array_pointer<T> allocate_array_zeroed(std::size_t count, int flags = 0) {
		auto arr = allocate_raw<T>(count);
		memset(arr, 0, count * sizeof(T));
		return arr;
	}

	template<class T>
	void deallocate_array(array_pointer<T>& p, std::size_t count) {
		deallocate_raw(p);
	}

	template<class T>
	pointer<T> allocate_node() {
		return allocate_raw<T>(1);
	}

	template<class T>
	void deallocate_node(pointer<T>& p) {
		deallocate_raw(p);
	}

	template<class T>
	static T* to_raw(T* p) {
		return p;
	}

	//stateless
	bool operator==(const base_allocator_to_eastl_raw&) const { return true; }
	bool operator!=(const base_allocator_to_eastl_raw&) const { return false; }
};



class allocator_to_eastl_vector_impl : public base_allocator_to_eastl_impl {
public:
	template<class T>
	static soft_ptr_impl<array_of<T>> to_soft(const array_pointer<T>& p) {
		if(p) {
			auto ptr = p.get_array_of_ptr();
			return { getControlBlock_( ptr ), ptr };
		}
		else
			return {};
	}
};

// we use raw pointers for stack iterators, and allocation, and
// soft_ptr_no_checks for user side pointers that go to the heap
class allocator_to_eastl_vector_raw : public base_allocator_to_eastl_raw {
public:
	template<class T>
	static soft_ptr_no_checks<array_of<T>> to_soft(T* p) {
		return { fbc_ptr_t(), p };
	}
};

template<memory_safety Safety>
using allocator_to_eastl_string = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_vector_impl, allocator_to_eastl_vector_raw>;

template<memory_safety Safety>
using allocator_to_eastl_vector = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_vector_impl, allocator_to_eastl_vector_raw>;


class allocator_to_eastl_hashtable_impl : public base_allocator_to_eastl_impl {
public:
	template<class T>
	static pointer<T> get_hashtable_sentinel() {
		return {make_zero_offset_t(), hashtable_sentinel<T>()};
	}

	template<class T>
	static array_pointer<T> get_empty_hashtable() {
		return {make_zero_offset_t(), empty_hashtable<T>()};
	}

	template<class T>
	static bool is_hashtable_sentinel(const pointer<T>& p) {
		return p.get_raw_ptr() == hashtable_sentinel<T>();
	}

	template<class T>
	static bool is_empty_hashtable(const array_pointer<T>& a) {
		return a.get_array_of_ptr() == empty_hashtable<T>();
	}

	template<class T>
	static bool is_hashtable_sentinel(const T* p) {
		return p == hashtable_sentinel<T>();
	}

	template<class T>
	static bool is_empty_hashtable(const T* a) {
		return a == empty_hashtable<T>();
	}


	// 'to_zero' works for node and for array
	template<class T>
	static pointer<T> to_base(const soft_ptr_impl<T>& p) {
			return { make_zero_offset_t(), p.getDereferencablePtr() };
	}

	template<class T>
	static soft_ptr_impl<T> to_soft(const pointer<T>& p) {
		if(p && !is_hashtable_sentinel(p)) {
			auto ptr = p.get_raw_ptr();
			return { getControlBlock_( ptr ), ptr };
		}
		else
			return {};
	}

	template<class T>
	static soft_ptr_impl<array_of<T>> to_soft(const array_pointer<T>& p) {
		if(p && !is_empty_hashtable(p)) {
			auto ptr = p.get_array_of_ptr();
			return { getControlBlock_( ptr ), ptr };
		}
		else
			return {};
	}
};


class allocator_to_eastl_hashtable_raw : public base_allocator_to_eastl_raw {
public:
	template<class T>
	static pointer<T> get_hashtable_sentinel() {
		return hashtable_sentinel<T>();
	}

	template<class T>
	static array_pointer<T> get_empty_hashtable() {
		return empty_hashtable_raw<T>();
	}

	template<class T>
	static bool is_hashtable_sentinel(const pointer<T>& p) {
		return p == hashtable_sentinel<T>();
	}

	template<class T>
	static bool is_empty_hashtable(const array_pointer<T>& a) {
		return a == empty_hashtable_raw<T>();
	}

	template<class T>
	static soft_ptr_no_checks<T> to_soft(pointer<T> p) {
		if(p && !is_hashtable_sentinel(p) && !is_empty_hashtable(p)) {
			return { fbc_ptr_t(), p };
		}
		else
			return {};
	}

	template<class T>
	static pointer<T> to_base(const soft_ptr_no_checks<T>& p) {
		return p.t;
	}
};

template<memory_safety Safety>
using allocator_to_eastl_hashtable = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_hashtable_impl,
			allocator_to_eastl_hashtable_raw>;


} // namespace safe_memory::detail

#endif // SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H
