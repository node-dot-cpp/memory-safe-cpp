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

#include <utility> //for std::swap
#include <safememory/detail/array_of.h>

namespace safememory::detail
{

struct make_zero_offset_t {};

/// \brief \c soft_ptr_with_zero_offset_base is not actually used and may be outdated
class soft_ptr_with_zero_offset_base
{
	friend class allocator_to_eastl_hashtable_impl;
	friend class allocator_to_eastl_hashtable_no_checks;
	
protected:
	void* ptr = nullptr;

public:
	soft_ptr_with_zero_offset_base() { }

	soft_ptr_with_zero_offset_base( void* raw ) :ptr(raw) { }

	soft_ptr_with_zero_offset_base( const soft_ptr_with_zero_offset_base& ) = default;
	soft_ptr_with_zero_offset_base& operator=( const soft_ptr_with_zero_offset_base& ) = default;
	soft_ptr_with_zero_offset_base( soft_ptr_with_zero_offset_base&& ) = default;
	soft_ptr_with_zero_offset_base& operator=( soft_ptr_with_zero_offset_base&& ) = default;

	soft_ptr_with_zero_offset_base( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_base& operator=( std::nullptr_t ) { reset(); return *this; }

	void swap( soft_ptr_with_zero_offset_base& other ) noexcept	{ std::swap(ptr, other.ptr); }

	explicit operator bool() const noexcept { return ptr != nullptr; }

	void reset() noexcept { ptr = nullptr; }

	bool operator == (const soft_ptr_with_zero_offset_base& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_base& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

};

/** \file
 * \brief Pointer wrappers to be used in libraries adaptation to \a safememory
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
template<class T>
class soft_ptr_with_zero_offset_impl
{
	T* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, T* raw ) : ptr(raw) {}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl& ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl& ) = default;

	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl&& ) = default;
	soft_ptr_with_zero_offset_impl<T>& operator=( soft_ptr_with_zero_offset_impl&& ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ) { reset(); return *this; }

	void reset() noexcept { ptr = nullptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }
	void swap( soft_ptr_with_zero_offset_impl& other ) noexcept	{ eastl::swap(ptr, other.ptr); }

	bool operator == (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

	T& operator*() const noexcept { return *get_raw_ptr(); }
	T* operator->() const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return ptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_impl();
};


/// \brief \c soft_ptr_with_zero_offset_no_checks is not actually used and may be outdated
template<class T>
class soft_ptr_with_zero_offset_no_checks
{
	T* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, T* raw ) : ptr(raw) {}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks& ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator=( soft_ptr_with_zero_offset_no_checks&& ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t ) { reset(); return *this; }

	void reset() noexcept { ptr = nullptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }
	void swap( soft_ptr_with_zero_offset_no_checks& other ) noexcept { eastl::swap(ptr, other.ptr); }

	bool operator == (const soft_ptr_with_zero_offset_no_checks& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_no_checks& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

	T& operator*() const noexcept { return *get_raw_ptr(); }
	T* operator->() const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return ptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_impl();
};


/// Template specialization of \c soft_ptr_with_zero_offset_impl for \c array_of<T>
template<class T>
class soft_ptr_with_zero_offset_impl<array_of<T>>
{
	array_of<T>* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, array_of<T>* raw ) :ptr(raw) {}

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( const soft_ptr_with_zero_offset_impl& other ) = default;

	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl&& other ) = default;
	soft_ptr_with_zero_offset_impl& operator=( soft_ptr_with_zero_offset_impl&& other ) = default;

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_impl& operator=( std::nullptr_t ){ reset(); return *this; }

	void reset() noexcept { ptr = nullptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }
	void swap( soft_ptr_with_zero_offset_impl& other ) noexcept { eastl::swap(ptr, other.ptr); }

	bool operator == (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_impl& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

	array_of<T>& operator*() const noexcept { return *get_array_of_ptr(); }
	array_of<T>* operator->() const noexcept { return get_array_of_ptr(); }
	array_of<T>* get_array_of_ptr() const noexcept { return ptr; }

	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return ptr ? get_array_of_ptr()->data() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_impl();
};

template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_impl<array_of<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_impl<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}

template<class T>
class soft_ptr_with_zero_offset_no_checks<array_of<T>>
{
	array_of<T>* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, array_of<T>* raw ) :ptr(raw) {}

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( const soft_ptr_with_zero_offset_no_checks& other ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& other ) = default;
	soft_ptr_with_zero_offset_no_checks& operator=( soft_ptr_with_zero_offset_no_checks&& other ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) { }
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t ){ reset(); return *this; }

	void reset() noexcept { ptr = nullptr; }
	explicit operator bool() const noexcept { return ptr != nullptr; }
	void swap( soft_ptr_with_zero_offset_no_checks& other ) noexcept { eastl::swap(ptr, other.ptr); }

	bool operator == (const soft_ptr_with_zero_offset_no_checks& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_no_checks& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

	array_of<T>& operator*() const noexcept { return *get_array_of_ptr(); }
	array_of<T>* operator->() const noexcept { return get_array_of_ptr(); }
	array_of<T>* get_array_of_ptr() const noexcept { return ptr; }

	T* operator+(std::ptrdiff_t n) const noexcept { return get_raw_begin() + n; }
	T& operator[](std::size_t n) const noexcept { return get_raw_begin()[n]; }
	T* get_raw_begin() const noexcept { return ptr ? get_array_of_ptr()->data() : nullptr; }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_impl();
};

template<class T>
std::ptrdiff_t operator-(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left - right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator-(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() - right;
}

template<class T>
std::ptrdiff_t operator==(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left == right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator==(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() == right;
}

template<class T>
std::ptrdiff_t operator!=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left != right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator!=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() != right;
}

template<class T>
std::ptrdiff_t operator<(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left < right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() < right;
}

template<class T>
std::ptrdiff_t operator<=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left <= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator<=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() <= right;
}

template<class T>
std::ptrdiff_t operator>(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left > right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() > right;
}

template<class T>
std::ptrdiff_t operator>=(const T* left, const soft_ptr_with_zero_offset_no_checks<array_of<T>>& right) {
	return left >= right.get_raw_begin();
}

template<class T>
std::ptrdiff_t operator>=(const soft_ptr_with_zero_offset_no_checks<array_of<T>>& left, const T* right) {
	return left.get_raw_begin() >= right;
}

template<class T, memory_safety S>
using soft_ptr_with_zero_offset = std::conditional_t<S == memory_safety::safe, 
			soft_ptr_with_zero_offset_impl<T>,
			soft_ptr_with_zero_offset_no_checks<T>>; 

} // namespace safememory::detail

#endif // SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H
