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

#include <safememory/safe_ptr.h>
#include <safememory/detail/soft_ptr_with_zero_offset.h>
#include <safememory/detail/flexible_array.h>
#include <type_traits>

/** \file
 * \brief Allocators feed by \a safememory containers into \c eastl ones.
 * 
 * These allocators hide the tricks required to make a non safety aware library like \c eastl
 * to work with safety features.
 * 
 * On \a safememory allocations hide an small piece of memory called \a ControlBlock in front
 * of every heap allocated object, that is used to keep track of \c soft_ptr pointing such object.
 * On normal user code, the \a ControlBlock is managed by the corresponding \c owning_ptr.
 * 
 * Here we have a set of allocation / deallocation functions, and functions to create \c soft_ptr
 * from allocated pointer, without the need to introduce \c owning_ptr semantics.
 * This is possible because \c eastl library is assumed to properly handle ownership of every
 * allocated pointer, and correctly pair allocation/deallocation calls.
 */ 

namespace safememory::detail {

extern flexible_array_with_memory<2, soft_ptr_with_zero_offset_base> gpSafeMemoryEmptyBucketArrayImpl;
extern flexible_array_with_memory<2, soft_ptr_with_zero_offset_base> gpSafeMemoryEmptyBucketArrayNoChecks;
extern void* gpSafeMemoryEmptyBucketArrayRaw[];


template<std::size_t alignment>
void* zombie_allocate_helper(std::size_t sz) {

	std::size_t head = sizeof(FirstControlBlock) - getPrefixByteCount();

	std::size_t total = head + sz;
	void* data =  zombieAllocateAligned<alignment>(total);

	void* dataForObj = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + head);
	
	auto cb = getControlBlock_(dataForObj);
	cb->init();

	return dataForObj;
}

#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND

template<memory_safety is_safe, std::size_t alignment>
std::pair<make_zero_offset_t, void*> allocate_helper(std::size_t sz) {

	if ( is_safe == memory_safety::none || g_CurrentAllocManager == nullptr ) {
		void* ptr = allocateAligned<alignment>(sz);
		typedef decltype(g_CurrentAllocManager->allocatorID()) id_type;
		id_type allocatorID = g_CurrentAllocManager != nullptr ? g_CurrentAllocManager->allocatorID() : 0;
		return { make_zero_offset_t{allocatorID}, ptr };
	}
	else {
		void* ptr = zombie_allocate_helper<alignment>(sz);
		auto allocatorID = g_CurrentAllocManager->allocatorID();
		return { make_zero_offset_t{allocatorID}, ptr };
	}
}

template<memory_safety is_safe, class T>
void deallocate_helper(const soft_ptr_with_zero_offset<T, is_safe>& p) {

	if (p) {
		T* dataForObj = p.get_raw_ptr();
		auto allocatorID = p.get_allocator_id();
		//we don't destruct here dataForObj->~T();
		if(is_safe == memory_safety::none || allocatorID == 0) {
			deallocate( dataForObj, alignof(T), allocatorID );
		}
		else {
			auto cb = getControlBlock_(dataForObj);
			cb->template updatePtrForListItemsWithInvalidPtr<T>();
			zombieDeallocate( getAllocatedBlock_(dataForObj), allocatorID );
			cb->clear(allocatorID);
		}
	}
}

#else

template<memory_safety is_safe, std::size_t alignment>
std::pair<make_zero_offset_t, void*> allocate_helper(std::size_t sz) {

	if ( is_safe == memory_safety::none ) {
		void* ptr = allocateAligned<alignment>(sz);
		return { make_zero_offset_t{}, ptr };
	}
	else {
		void* ptr = zombie_allocate_helper<alignment>(sz);
		return { make_zero_offset_t{}, ptr };
	}
}

template<memory_safety is_safe, class T>
void deallocate_helper(const soft_ptr_with_zero_offset<T, is_safe>& p) {

	if (p) {
		T* dataForObj = p.get_raw_ptr();
		//we don't destruct here dataForObj->~T();
		if(is_safe == memory_safety::none) {
			deallocate( dataForObj, alignof(T) );
		}
		else {
			auto cb = getControlBlock_(dataForObj);
			cb->template updatePtrForListItemsWithInvalidPtr<T>();
			zombieDeallocate( getAllocatedBlock_(dataForObj) );
			cb->clear();
		}
	}
}


#endif

template<memory_safety is_safe, class T>
soft_ptr_with_zero_offset<T, is_safe> allocate_node_helper() {

	auto mm = allocate_helper<is_safe, alignof(T)>(sizeof(T));

	auto dataForObj = reinterpret_cast<T*>(mm.second);

	return { mm.first, dataForObj };
}

template<memory_safety is_safe, typename T, bool zeroed>
soft_ptr_with_zero_offset<flexible_array<T>, is_safe> allocate_flexible_array_helper(std::size_t count) {

	static_assert(std::is_trivially_destructible_v<flexible_array<T>>, "flexible_array must be trivially destructible");
	
	auto total = flexible_array<T>::calculateSize(count);

	auto mm = allocate_helper<is_safe, alignof(flexible_array<T>)>(total);

	if constexpr (zeroed)
		std::memset(mm.second, 0, total);

	::new ( mm.second ) flexible_array<T>(count);

	auto dataForObj = reinterpret_cast<flexible_array<T>*>(mm.second);

	return { mm.first, dataForObj };
}


// two special values used inside eastl::hashtable
// soft_ptr needs to have special behaviour around this values.

template<class T>
constexpr T* hashtable_sentinel() {
	//mb: we don't use NODECPP_SECOND_NULLPTR or original eastl value
	// because now soft_ptr_with_zero_offset uses allocptr_with_zombie_property_and_data
	// and such can't acomodate any value, as lower and higer bits are used for other pourposes
	return reinterpret_cast<T*>((uintptr_t)8);
}

template<class T>
constexpr T* empty_hashtable_impl() {
	return reinterpret_cast<T*>(&gpSafeMemoryEmptyBucketArrayImpl);
}

template<class T>
constexpr T* empty_hashtable_no_checks() {
	return reinterpret_cast<T*>(&gpSafeMemoryEmptyBucketArrayNoChecks);
}

template<class T>
constexpr T* empty_hashtable_raw() {
	return reinterpret_cast<T*>(&gpSafeMemoryEmptyBucketArrayRaw);
}

/// helper class, friend of \c soft_ptr_impl and \c soft_ptr_no_checks to access private stuff
class soft_ptr_helper {
	public:

	template<class T>
	static T* to_raw(const soft_ptr_impl<T>& p) {
		return p.getDereferencablePtr();
	}

	template<class T>
	static T* to_raw(const soft_ptr_no_checks<T>& p) {
		return p.t;
	}

	template<class T>
	static soft_ptr_impl<T> make_soft_ptr_impl(FirstControlBlock* cb, T* t) {
		return soft_ptr_impl<T>(cb, t);
	}

	template<class T>
	static soft_ptr_no_checks<T> make_soft_ptr_no_checks(fbc_ptr_t cb, T* t) {
		return soft_ptr_no_checks<T>(cb, t);
	}
};

class soft_this_ptr_raii_impl {

	void* stackTmp;
public:
	soft_this_ptr_raii_impl(void* ptr) noexcept : stackTmp(thg_stackPtrForMakeOwningCall) {
		thg_stackPtrForMakeOwningCall = ptr;
	}

	~soft_this_ptr_raii_impl() {
		thg_stackPtrForMakeOwningCall = stackTmp;
	}
};

class soft_this_ptr_raii_dummy {
public:
	soft_this_ptr_raii_dummy(void* ptr) {}
};

// we can safely use a dummy when the type is trivial.
template<class T>
using soft_this_ptr_raii = std::conditional_t<!std::is_trivially_copyable_v<T>,
			soft_this_ptr_raii_impl, soft_this_ptr_raii_dummy>;

class base_allocator_to_eastl_impl {
public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	template<class T>
	using pointer = soft_ptr_with_zero_offset_impl<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_impl<flexible_array<T>>;

	template<class T>
	using soft_pointer = soft_ptr_impl<T>;

	template<class T>
	using soft_array_pointer = soft_ptr_impl<flexible_array<T>>;

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0) {
		// for non trivial types, always zero memory
		return allocate_flexible_array_helper<is_safe, T, !std::is_trivially_copyable_v<T>>(count);
	}

	template<class T>
	array_pointer<T> allocate_array_zeroed(std::size_t count, int flags = 0) {
		return allocate_flexible_array_helper<is_safe, T, true>(count);
	}

	template<class T>
	void deallocate_array(const array_pointer<T>& p, std::size_t count) {
		deallocate_helper<is_safe, flexible_array<T>>(p);
	}

	template<class T>
	pointer<T> allocate_node() {
		return allocate_node_helper<is_safe, T>();
	}

	template<class T>
	void deallocate_node(const pointer<T>& p) {
		deallocate_helper<is_safe, T>(p);
	}

	template<class T>
	static T* to_raw(const pointer<T>& p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static T* to_raw(const array_pointer<T>& p) {
		return p.get_raw_begin();
	}

#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
	template<class T>
	static soft_this_ptr_raii<T> make_raii(const pointer<T>& p) {
		if(p.get_allocator_id() != 0)
			return {p.get_raw_ptr()};
		else
			return {nullptr};
	}
#else
	template<class T>
	static soft_this_ptr_raii<T> make_raii(const pointer<T>& p) {
		return {p.get_raw_ptr()};
	}
#endif


	template<class T>
	static pointer<T> get_hashtable_sentinel() {
		return {make_zero_offset_t{SAFEMEMORY_INVALID_ALLOCATOR}, hashtable_sentinel<T>()};
	}

	template<class T>
	static array_pointer<T> get_empty_hashtable() {
		return {make_zero_offset_t{SAFEMEMORY_INVALID_ALLOCATOR}, empty_hashtable_impl<flexible_array<T>>()};
	}

	template<class T>
	static bool is_hashtable_sentinel(const pointer<T>& p) {
		return p.get_raw_ptr() == hashtable_sentinel<T>();
	}

	template<class T>
	static bool is_empty_hashtable(const pointer<T>& a) {
		return a.get_raw_ptr() == empty_hashtable_impl<T>();
	}

	template<class T>
	static soft_ptr_impl<T> to_soft(const pointer<T>& p) {
		if(p) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, !is_hashtable_sentinel(p));
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, !is_empty_hashtable(p));
			auto ptr = p.get_raw_ptr();
#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
			auto allocatorID = p.get_allocator_id();
			auto cb = p.get_allocator_id() != 0 ? getControlBlock_(ptr) : nullptr;
#else
			auto cb = getControlBlock_(ptr);
#endif
			return soft_ptr_helper::make_soft_ptr_impl(cb , ptr);
		}
		else
			return {};
	}

	static void force_changes_in_dtor(const void*) { forcePreviousChangesToThisInDtor(ptr); }

	//stateless
	bool operator==(const base_allocator_to_eastl_impl&) const { return true; }
	bool operator!=(const base_allocator_to_eastl_impl&) const { return false; }
};

class base_allocator_to_eastl_no_checks {
public:

	static constexpr memory_safety is_safe = memory_safety::none; 

	template<class T>
	using pointer = soft_ptr_with_zero_offset_no_checks<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_no_checks<flexible_array<T>>;

	template<class T>
	using soft_pointer = soft_ptr_no_checks<T>;

	template<class T>
	using soft_array_pointer = soft_ptr_no_checks<flexible_array<T>>;

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0) {
		return allocate_flexible_array_helper<is_safe, T, false>(count);
	}

	template<class T>
	array_pointer<T> allocate_array_zeroed(std::size_t count, int flags = 0) {
		return allocate_flexible_array_helper<is_safe, T, true>(count);
	}

	template<class T>
	void deallocate_array(const array_pointer<T>& p, std::size_t count) {
		deallocate_helper<is_safe, flexible_array<T>>(p);
	}

	template<class T>
	pointer<T> allocate_node() {
		return allocate_node_helper<is_safe, T>();
	}

	template<class T>
	void deallocate_node(const pointer<T>& p) {
		deallocate_helper<is_safe, T>(p);
	}

	template<class T>
	static T* to_raw(const pointer<T>& p) {
		return p.get_raw_ptr();
	}

	template<class T>
	static T* to_raw(const array_pointer<T>& p) {
		return p.get_raw_begin();
	}

	// We don't have a ControlBlock, so soft_this_ptr can't possible work
	template<class T>
	static soft_this_ptr_raii_dummy make_raii(const pointer<T>& p) {
		return {nullptr};
	}

	template<class T>
	static pointer<T> get_hashtable_sentinel() {
		return {make_zero_offset_t{SAFEMEMORY_INVALID_ALLOCATOR}, hashtable_sentinel<T>()};
	}

	template<class T>
	static array_pointer<T> get_empty_hashtable() {
		return {make_zero_offset_t{SAFEMEMORY_INVALID_ALLOCATOR}, empty_hashtable_no_checks<flexible_array<T>>()};
	}

	template<class T>
	static bool is_hashtable_sentinel(const pointer<T>& p) {
		return p.get_raw_ptr() == hashtable_sentinel<T>();
	}

	template<class T>
	static bool is_empty_hashtable(const pointer<T>& a) {
		return a.get_raw_ptr() == empty_hashtable_no_checks<T>();
	}


	template<class T>
	static soft_ptr_no_checks<T> to_soft(const pointer<T>& p) {
		if(p) {
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, !is_hashtable_sentinel(p));
			NODECPP_ASSERT(module_id, nodecpp::assert::AssertLevel::regular, !is_empty_hashtable(p));
			auto ptr =p.get_raw_ptr();
			return soft_ptr_helper::make_soft_ptr_no_checks(fbc_ptr_t(), ptr);
		}
		else
			return {};
	}

	static void force_changes_in_dtor(const void*) {}

	//stateless
	bool operator==(const base_allocator_to_eastl_no_checks&) const { return true; }
	bool operator!=(const base_allocator_to_eastl_no_checks&) const { return false; }
};



template<memory_safety Safety>
using allocator_to_eastl_string = std::conditional_t<Safety == memory_safety::safe,
			base_allocator_to_eastl_impl, base_allocator_to_eastl_no_checks>;

template<memory_safety Safety>
using allocator_to_eastl_vector = std::conditional_t<Safety == memory_safety::safe,
			base_allocator_to_eastl_impl, base_allocator_to_eastl_no_checks>;



template<memory_safety Safety>
using allocator_to_eastl_hashtable = std::conditional_t<Safety == memory_safety::safe,
			base_allocator_to_eastl_impl, base_allocator_to_eastl_no_checks>;


} // namespace safememory::detail

#endif // SAFE_MEMORY_DETAIL_ALLOCATOR_TO_EASTL_H
