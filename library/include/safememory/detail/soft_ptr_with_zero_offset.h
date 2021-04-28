/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
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

#ifndef SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H
#define SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H

#include <safememory/detail/flexible_array.h>

/** \file
 * \brief Pointer wrappers to be used in libraries adaptation to \a safememory
 * 
 * \c soft_ptr_with_zero_offset_impl is a pointer wrapper used as a bridge between our safety aware code
 * and \c eastl libraries that know nothing about \c soft_ptr_impl
 * 
 * On \a safememory allocations, an special chunk of memory called \a ControlBlock
 * is allocated in front of every object on the heap.
 * Also allocations always return an \c owning_ptr_impl that is aware or such \a ControlBlock
 * and use it to keep track of \c soft_ptr created from him.
 * 
 * Since \c owning_ptr_impl has ownership sematics is difficult to modify existing libraries like
 * \c eastl containers to use it.
 * Then \c soft_ptr_with_zero_offset_impl is used to fill the gap. Is a pointer wrapper returned
 * by allocations intended to be used by such libraries, has a \a ControlBlock,
 * but has raw pointer semantics, minimizing the changes needed to make such ports.
 * 
 * Allocator knows how to properly create \c soft_ptr_impl from it. 
 * 
 * It is assumed that existing libraries are bug free and correctly pair allocation/deallocation
 * without the need of \c owning_ptr_impl help.
 * 
 * Also \c soft_ptr_with_zero_offset_impl and \c soft_ptr_with_zero_offset_no_checks are identicall
 * but we keep them as separate classes to detect unintended errors
 */

namespace safememory::detail
{


#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
struct make_zero_offset_t {
	uint16_t allocatorID; 
};
#else
struct make_zero_offset_t {};
#endif


/// \brief \c soft_ptr_with_zero_offset_base is not actually used and may be outdated
class soft_ptr_with_zero_offset_base
{
public:
	friend class allocator_to_eastl_hashtable_impl;
	friend class allocator_to_eastl_hashtable_no_checks;
	
#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
	using base_pointer_t = nodecpp::platform::ptrwithdatastructsdefs::generic_allocptr_with_zombie_property_and_data_; 
#else
	using base_pointer_t = nodecpp::platform::allocptr_with_zombie_property_and_data; 
#endif // SAFE_PTR_DEBUG_MODE
private:
	base_pointer_t ptr;
#else // NODECPP_MEMORY_SAFETY_ON_DEMAND
private:
	void* ptr = nullptr;
#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND


public:
	soft_ptr_with_zero_offset_base() { }

#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
	soft_ptr_with_zero_offset_base( make_zero_offset_t id, void* raw ) { ptr.init(raw, id.allocatorID); }
	soft_ptr_with_zero_offset_base( const soft_ptr_with_zero_offset_base& other ) {
		ptr.copy_from(other.ptr);
	} 
	
	soft_ptr_with_zero_offset_base& operator=( const soft_ptr_with_zero_offset_base& other ) {
		if(this == std::addressof(other))
			return *this;

		ptr.copy_from(other.ptr);
		return *this;
	}

	soft_ptr_with_zero_offset_base( soft_ptr_with_zero_offset_base&& other ) {
		// mb: copy is actually more light weight than move, and we don't need real move
		ptr.copy_from(other.ptr);
	}
	
	soft_ptr_with_zero_offset_base& operator=( soft_ptr_with_zero_offset_base&& other ) {
		// mb: copy is actually more light weight than move, and we don't need real move
		if(this == std::addressof(other))
			return *this;
	
		ptr.copy_from(other.ptr);
		return *this;
	}

	uint16_t get_allocator_id() const noexcept { return ptr.get_data(); }
	void* get_raw() const noexcept { return ptr.get_ptr(); }
	void reset() noexcept { ptr.init(nullptr, 0); }
	void swap( soft_ptr_with_zero_offset_base& other ) noexcept	{ ptr.swap(other.ptr); }

#else
	soft_ptr_with_zero_offset_base( make_zero_offset_t, void* raw ) : ptr(raw) {}
	soft_ptr_with_zero_offset_base( const soft_ptr_with_zero_offset_base& ) = default;
	soft_ptr_with_zero_offset_base& operator=( const soft_ptr_with_zero_offset_base& ) = default;
	soft_ptr_with_zero_offset_base( soft_ptr_with_zero_offset_base&& ) = default;
	soft_ptr_with_zero_offset_base& operator=( soft_ptr_with_zero_offset_base&& ) = default;

	void* get_raw() const noexcept { return ptr; }
	void reset() noexcept { ptr = nullptr; }
	void swap( soft_ptr_with_zero_offset_base& other ) noexcept	{
		void* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}
#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND

//	soft_ptr_with_zero_offset_base( void* raw ) :ptr(raw) { }

	soft_ptr_with_zero_offset_base( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_base& operator=( std::nullptr_t ) { reset(); return *this; }

	explicit operator bool() const noexcept { return get_raw() != nullptr; }

	bool operator == (const soft_ptr_with_zero_offset_base& other ) const noexcept { return get_raw() == other.get_raw(); }
	bool operator != (const soft_ptr_with_zero_offset_base& other ) const noexcept { return get_raw() != other.get_raw(); }

	bool operator == (std::nullptr_t) const noexcept { return get_raw() == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return get_raw() != nullptr; }


	// mb: destructor should be trivial to allow use in unions
	~soft_ptr_with_zero_offset_base() = default;
};

#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
#define SAFEMEMORY_INVALID_ALLOCATOR soft_ptr_with_zero_offset_base::base_pointer_t::max_data
#else
#define SAFEMEMORY_INVALID_ALLOCATOR
#endif


template<class T>
class soft_ptr_with_zero_offset_impl : public soft_ptr_with_zero_offset_base
{
public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t id, T* raw ) : soft_ptr_with_zero_offset_base(id, raw) {}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl& ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl& ) = default;

	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl&& ) = default;
	soft_ptr_with_zero_offset_impl<T>& operator=( soft_ptr_with_zero_offset_impl&& ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ) { reset(); return *this; }

	T& operator*() const noexcept { return *get_raw_ptr(); }
	T* operator->() const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return reinterpret_cast<T*>(get_raw()); }

	// mb: destructor should be trivial to allow use in unions
	~soft_ptr_with_zero_offset_impl() = default;
};


/// \brief \c soft_ptr_with_zero_offset_no_checks is not actually used and may be outdated
template<class T>
class soft_ptr_with_zero_offset_no_checks : public soft_ptr_with_zero_offset_base
{
public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t id, T* raw ) : soft_ptr_with_zero_offset_base(id, raw) {}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks& ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator=( soft_ptr_with_zero_offset_no_checks&& ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t ) { reset(); return *this; }

	T& operator*() const noexcept { return *get_raw_ptr(); }
	T* operator->() const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return reinterpret_cast<T*>(get_raw()); }

	// mb: destructor should be trivial to allow use in unions
	~soft_ptr_with_zero_offset_no_checks() = default;
};


/// Template specialization of \c soft_ptr_with_zero_offset_impl for \c flexible_array<T>
template<class T>
class soft_ptr_with_zero_offset_impl<flexible_array<T>> : public soft_ptr_with_zero_offset_base
{
public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t id, flexible_array<T>* raw ) : soft_ptr_with_zero_offset_base(id, raw) {}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl& other ) = default;

	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl&& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( soft_ptr_with_zero_offset_impl&& other ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ){ reset(); return *this; }


	flexible_array<T>& operator*() const noexcept { return *get_raw_ptr(); }
	flexible_array<T>* operator->() const noexcept { return get_raw_ptr(); }
	flexible_array<T>* get_array_of_ptr() const noexcept { return get_raw_ptr(); }
	flexible_array<T>* get_raw_ptr() const noexcept { return reinterpret_cast<flexible_array<T>*>(get_raw()); }

	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return get_raw_ptr() ? get_raw_ptr()->data() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	~soft_ptr_with_zero_offset_impl() = default;
};

template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_impl<flexible_array<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_impl<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}

template<class T>
class soft_ptr_with_zero_offset_no_checks<flexible_array<T>> : public soft_ptr_with_zero_offset_base
{
public:
	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t id, flexible_array<T>* raw ) : soft_ptr_with_zero_offset_base(id, raw) {}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks& other ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( soft_ptr_with_zero_offset_no_checks&& other ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t ){ reset(); return *this; }

	flexible_array<T>& operator*() const noexcept { return *get_raw_ptr(); }
	flexible_array<T>* operator->() const noexcept { return get_raw_ptr(); }
	flexible_array<T>* get_array_of_ptr() const noexcept { return get_raw_ptr(); }
	flexible_array<T>* get_raw_ptr() const noexcept { return reinterpret_cast<flexible_array<T>*>(get_raw()); }

	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return get_raw_ptr() ? get_raw_ptr()->data() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	~soft_ptr_with_zero_offset_no_checks() = default;
};

template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_no_checks<flexible_array<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}

template<class T, memory_safety S>
using soft_ptr_with_zero_offset = std::conditional_t<S == memory_safety::safe, 
			soft_ptr_with_zero_offset_impl<T>,
			soft_ptr_with_zero_offset_no_checks<T>>; 

} // namespace safememory::detail

#endif // SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H
