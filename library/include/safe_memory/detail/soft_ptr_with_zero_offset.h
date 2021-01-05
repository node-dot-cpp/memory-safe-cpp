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

// #include <safe_ptr.h>
// #include <safe_ptr_common.h>


namespace safe_memory::detail
{

struct make_zero_offset_t {};

// mb: soft_ptr_with_zero_offset_base is not actually used and may be outdated
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

	void swap( soft_ptr_with_zero_offset_base& other ) noexcept
	{
		void* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	explicit operator bool() const noexcept { return ptr != nullptr; }

	void reset() noexcept { ptr = nullptr; }

	bool operator == (const soft_ptr_with_zero_offset_base& other ) const noexcept { return ptr == other.ptr; }
	bool operator != (const soft_ptr_with_zero_offset_base& other ) const noexcept { return ptr != other.ptr; }

	bool operator == (std::nullptr_t) const noexcept { return ptr == nullptr; }
	bool operator != (std::nullptr_t) const noexcept { return ptr != nullptr; }

};

template<class T>
class soft_ptr_with_zero_offset_impl
{
	// template<class TT>
	// friend class soft_ptr_with_zero_offset_impl;

	// template<class TT>
	// friend void deallocate_impl(soft_ptr_with_zero_offset_impl<TT>&);

	// friend class allocator_to_eastl_hashtable_impl;
	// friend class allocator_to_eastl_hashtable_no_checks;

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
	void swap( soft_ptr_with_zero_offset_impl& other ) noexcept	{
		T* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

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


// mb: soft_ptr_with_zero_offset_no_checks is not actually used and may be outdated
template<class T>
class soft_ptr_with_zero_offset_no_checks : public soft_ptr_with_zero_offset_base
{
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	template<class TT>
	friend void deallocate_no_checks(soft_ptr_with_zero_offset_no_checks<TT>&);


public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() { }

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, T* raw ) : soft_ptr_with_zero_offset_base(raw) {}


	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator=( const soft_ptr_with_zero_offset_no_checks& ) = default;

	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks&& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator=( soft_ptr_with_zero_offset_no_checks&& ) = default;

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_no_checks& operator=( std::nullptr_t )
		{ soft_ptr_with_zero_offset_base::reset(); return *this; }

	using soft_ptr_with_zero_offset_base::reset;
	using soft_ptr_with_zero_offset_base::swap;
	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::operator ==;
	using soft_ptr_with_zero_offset_base::operator !=;

	T& operator*() const noexcept { return *get_raw_ptr(); }
	T* operator->() const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return reinterpret_cast<T*>(ptr); }

	// mb: destructor should be trivial to allow use in unions
	// ~soft_ptr_with_zero_offset_no_checks();
};

template<class T, memory_safety S>
using soft_ptr_with_zero_offset = std::conditional_t<S == memory_safety::safe, 
			soft_ptr_with_zero_offset_impl<T>,
			soft_ptr_with_zero_offset_no_checks<T>>; 

} // namespace safe_memory::detail

#endif // SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H
