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



template<class T, bool bConst, memory_safety Safety>
using safe_array_iterator2 = std::conditional_t<Safety == memory_safety::safe, 
			array_of_iterator_impl<T, bConst, soft_ptr_with_zero_offset_impl>,
			array_of_iterator_no_checks<T, bConst>>;


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
		T* dataForObj = p.operator->();
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
		array_of<T>* dataForObj = p.operator->();
		dataForObj->~array_of<T>();
		auto cb = getControlBlock_(dataForObj);
		cb->template updatePtrForListItemsWithInvalidPtr<array_of<T>>();
		zombieDeallocate( getAllocatedBlock_(dataForObj) );
		cb->clear();
	}
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
		T* dataForObj = p.operator->();
		//dataForObj->~T(); //don't destruct here
		deallocate(dataForObj);
	}
}

template<class T>
void deallocate_array_no_checks(soft_ptr_with_zero_offset_no_checks<array_of<T>>& p) {

	if (p) {
		array_of<T>* dataForObj = p.operator->();
		dataForObj->~array_of<T>();
		deallocate(dataForObj);
	}
}



class allocator_to_eastl_impl {
public:
	static constexpr memory_safety is_safe = memory_safety::safe; 
	
	template<class T>
	class pointer_types {
	public:
		typedef soft_ptr_with_zero_offset_impl<T> pointer;
		typedef soft_ptr_with_zero_offset_impl<array_of<T>> array;
		typedef array_of_iterator_impl<T, false, soft_ptr_impl> array_iterator;
		typedef array_of_iterator_impl<T, true, soft_ptr_impl> const_array_iterator;
	};
	
	template<class T>
	typename pointer_types<T>::array allocate_array(std::size_t count, int flags = 0) {
		return allocate_array_impl<T>(count);
	}

	template<class T>
	typename pointer_types<T>::array allocate_array_zeroed(std::size_t count, int flags = 0) {
		return allocate_array_impl<T, true>(count);
	}

	template<class T>
	void deallocate_array(soft_ptr_with_zero_offset_impl<array_of<T>> p, std::size_t count) {
		deallocate_array_impl(p);
	}

	template<class T>
	typename pointer_types<T>::pointer allocate() {
		return allocate_impl<T>();
	}

	template<class T>
	void deallocate(soft_ptr_with_zero_offset_impl<T> p) {
		deallocate_impl(p);
	}

	template<class T>
	typename pointer_types<T>::pointer get_hashtable_sentinel() const {
		return soft_ptr_with_zero_offset_impl<T>(make_zero_offset_t(), reinterpret_cast<T*>((uintptr_t)~0));
	}

	template<class T>
	typename pointer_types<T>::array get_empty_hashtable() const {
		return soft_ptr_with_zero_offset_impl<array_of<T>>(make_zero_offset_t(), reinterpret_cast<array_of<T>*>(&gpSafeMemoryEmptyBucketArray));
	}

	template<class T>
	static T* to_raw(soft_ptr_with_zero_offset_impl<T> p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static T* to_raw(soft_ptr_with_zero_offset_impl<array_of<T>> p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static soft_ptr_impl<T> to_soft(const soft_ptr_with_zero_offset_impl<T>& p) {
		void* ptr = p.ptr;
		if(ptr == nullptr) 
			return {};
		else if(ptr == reinterpret_cast<void*>((uintptr_t)~0))
			return {};
		else if(ptr == &gpSafeMemoryEmptyBucketArray)
			return {};
		else {
			FirstControlBlock* cb = getControlBlock_( ptr );
			return { cb, reinterpret_cast<T*>(ptr) };
		}
	}

	template<class T>
	static soft_ptr_with_zero_offset_impl<T> to_zero(const soft_ptr_impl<T>& p) {
		return { make_zero_offset_t(), p.getDereferencablePtr() };
	}

	bool operator==(const allocator_to_eastl_impl&) const { return true; }
	bool operator!=(const allocator_to_eastl_impl&) const { return false; }
};

class allocator_to_eastl_no_checks {
public:
	static constexpr memory_safety is_safe = memory_safety::none; 

	template<class T>
	class pointer_types {
	public:
		typedef soft_ptr_with_zero_offset_no_checks<T> pointer;
		typedef soft_ptr_with_zero_offset_no_checks<array_of<T>> array;
		typedef array_of_iterator_no_checks<T, false> array_iterator;
		typedef array_of_iterator_no_checks<T, true> const_array_iterator;
	};

	template<class T>
	typename pointer_types<T>::pointer allocate() {
		return allocate_no_checks<T>();
	}

	template<class T>
	void deallocate(soft_ptr_with_zero_offset_no_checks<T> p) {
		deallocate_no_checks(p);
	}

	template<class T>
	typename pointer_types<T>::array allocate_array(std::size_t count, int flags = 0) {
		return allocate_array_no_checks<T>(count);
	}

	template<class T>
	typename pointer_types<T>::array allocate_array_zeroed(std::size_t count, int flags = 0) {
		auto arr = allocate_array<T>(count);
		memset(arr->begin(), 0, count * sizeof(T));
		return arr;
	}

	template<class T>
	void deallocate_array(soft_ptr_with_zero_offset_no_checks<array_of<T>> p, std::size_t count) {
		deallocate_array_no_checks(p);
	}

	//used by hashtable to mark 'end()'
	template<class T>
	typename pointer_types<T>::pointer get_hashtable_sentinel() const {
		return soft_ptr_with_zero_offset_no_checks<T>(make_zero_offset_t(), reinterpret_cast<T*>((uintptr_t)~0));
	}

	template<class T>
	typename pointer_types<T>::array get_empty_hashtable() const {
		return soft_ptr_with_zero_offset_no_checks<array_of<T>>(make_zero_offset_t(), reinterpret_cast<array_of<T>*>(&gpSafeMemoryEmptyBucketArray));
	}

	template<class T>
	static T* to_raw(soft_ptr_with_zero_offset_no_checks<T> p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static T* to_raw(soft_ptr_with_zero_offset_no_checks<array_of<T>> p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static soft_ptr_no_checks<T> to_soft(const soft_ptr_with_zero_offset_no_checks<T>& p) {
		return { fbc_ptr_t(), reinterpret_cast<T*>(p.ptr) };
	}

	template<class T>
	static soft_ptr_with_zero_offset_no_checks<T> to_zero(const soft_ptr_no_checks<T>& p) {
		return { make_zero_offset_t(), p.t };
	}

	bool operator==(const allocator_to_eastl_no_checks&) const { return true; }
	bool operator!=(const allocator_to_eastl_no_checks&) const { return false; }
};

template<memory_safety Safety>
using allocator_to_eastl = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_impl,
			allocator_to_eastl_no_checks>;



template<class T>
class allocator_to_eastl_string_impl {
public:
	static constexpr memory_safety is_safe = memory_safety::safe; 
	
	typedef soft_ptr_with_zero_offset_impl<array_of<T>> array_pointer;
	
	array_pointer allocate_array(std::size_t count, int flags = 0) {
		return allocate_array_impl<T>(count);
	}

	array_pointer allocate_array_zeroed(std::size_t count, int flags = 0) {
		return allocate_array_impl<T, true>(count);
	}

	void deallocate_array(array_pointer p, std::size_t count) {
		deallocate_array_impl(p);
	}

	static T* to_raw(array_pointer p) {
		return p.get_raw_ptr();
	}

	bool operator==(const allocator_to_eastl_string_impl&) const { return true; }
	bool operator!=(const allocator_to_eastl_string_impl&) const { return false; }
};

template<class T>
class allocator_to_eastl_string_no_checks {
public:
	static constexpr memory_safety is_safe = memory_safety::none; 

	typedef T* array_pointer;

	T* allocate_array(std::size_t count, int flags = 0) {
		return reinterpret_cast<T*>(allocate(sizeof(T) * count)); 
	}

	T* allocate_array_zeroed(std::size_t count, int flags = 0) {
		void* arr = allocate(count * sizeof(T));
		memset(arr, 0, count * sizeof(T));
		return reinterpret_cast<T*>(arr);
	}

	void deallocate_array(T* p, std::size_t count) {
		deallocate(p);
	}

	static
	T* to_raw(T* p) { return p; }

	bool operator==(const allocator_to_eastl_string_no_checks&) const { return true; }
	bool operator!=(const allocator_to_eastl_string_no_checks&) const { return false; }
};

template<typename T, memory_safety Safety>
using allocator_to_eastl_string = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_string_impl<T>,
			allocator_to_eastl_string_no_checks<T>>;



} // namespace safe_memory::detail

#endif // SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H
