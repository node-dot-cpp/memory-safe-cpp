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

#include <safe_ptr.h>
#include <safe_ptr_common.h>


namespace safe_memory::detail
{

using nodecpp::safememory::getControlBlock_;
using nodecpp::safememory::FirstControlBlock;

struct make_zero_offset_t {};

class soft_ptr_with_zero_offset_base
{
	friend class allocator_to_eastl_impl;
	friend class allocator_to_eastl_no_checks;
	
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
class soft_ptr_with_zero_offset_impl : public soft_ptr_with_zero_offset_base
{
	// friend class owning_ptr_impl<T>;

	// friend struct ::nodecpp::safememory::FirstControlBlock;

	// template<class TT>
	// friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_impl;

	template<class TT>
	friend void deallocate_impl(soft_ptr_with_zero_offset_impl<TT>&);

	template<class TT>
	friend soft_ptr_impl<TT> zero_to_soft(const soft_ptr_with_zero_offset_impl<TT>&);


	// T* ptr= nullptr;

	// FirstControlBlock* getControlBlock() const { return getControlBlock_( ptr ); }
	// static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( make_zero_offset_t, T* raw ) : soft_ptr_with_zero_offset_base(raw) {}
	// {
	// 	ptr = raw;
	// }

	// soft_ptr_with_zero_offset_impl( const owning_ptr_impl<T>& owner )
	// {
	// 	ptr = owner.t.getTypedPtr();
	// }
	// soft_ptr_with_zero_offset_impl<T>& operator = ( const owning_ptr_impl<T>& owner )
	// {
	// 	ptr = owner.t.getTypedPtr();
	// 	return *this;
	// }

	// soft_ptr_with_zero_offset_impl( const owning_ptr_base_impl<T>& owner )
	// {
	// 	ptr = owner.t.getTypedPtr();
	// }
	// soft_ptr_with_zero_offset_impl<T>& operator = ( const owning_ptr_base_impl<T>& owner )
	// {
	// 	ptr = owner.t.getTypedPtr();
	// 	return *this;
	// }

	// soft_ptr_with_zero_offset_impl( const soft_ptr_impl<T>& other )
	// {
	// 	ptr = other.getDereferencablePtr();
	// }
	// soft_ptr_with_zero_offset_impl<T>& operator = ( const soft_ptr_impl<T>& other )
	// {
	// 	ptr = other.getDereferencablePtr();
	// 	return *this;
	// }

	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl<T>& ) = default;
	// {
	// 	ptr = other.ptr;
	// }
	soft_ptr_with_zero_offset_impl<T>& operator = ( const soft_ptr_with_zero_offset_impl<T>& ) = default;
	// {
	// 	ptr = other.ptr;
	// 	return *this;
	// }


	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl<T>&& ) = default;
	// {
	// 	// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
	// 	if ( this == &other ) return;
	// 	ptr = other.ptr;
	// }

	soft_ptr_with_zero_offset_impl<T>& operator = ( soft_ptr_with_zero_offset_impl<T>&& ) = default;
	// {
	// 	// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
	// 	if ( this == &other ) return *this;
	// 	ptr = other.ptr;
	// 	return *this;
	// }

	soft_ptr_with_zero_offset_impl( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_impl& operator = ( std::nullptr_t )
		{ soft_ptr_with_zero_offset_base::reset(); return *this; }
	// {
	// 	reset();
	// 	return *this;
	// }

	// void swap( soft_ptr_with_zero_offset_impl<T>& other )
	// {
	// 	T* tmp = ptr;
	// 	ptr = other.ptr;
	// 	other.ptr = tmp;
	// }
	using soft_ptr_with_zero_offset_base::swap;

	// soft_ptr_impl<T> get_soft() const
	// {
	// 	if(NODECPP_UNLIKELY(ptr == reinterpret_cast<T*>((uintptr_t)~0))) {
	// 		//mb: ~0 is special value used by eastl hashtable to mean end()
	// 		// we may want to return a nullptr or a zombie ptr
	// 		return soft_ptr_impl<T>();
	// 	}
	// 	else if(NODECPP_LIKELY(ptr != nullptr)) {
	// 		FirstControlBlock* cb = getControlBlock_( ptr );
	// 		return soft_ptr_impl<T>( cb, ptr );
	// 	}
	// 	else
	// 		return soft_ptr_impl<T>();
	// }

	// explicit operator bool() const noexcept
	// {
	// 	return ptr != nullptr;
	// }

	// void reset() { ptr = nullptr; }
	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::reset;

	// bool operator == (const owning_ptr_impl<T>& other ) const { return ptr == other.t.getTypedPtr(); }
	// template<class T1> 
	// bool operator == (const owning_ptr_impl<T1>& other ) const { return ptr == other.t.getTypedPtr(); }

	// bool operator == (const soft_ptr_impl<T>& other ) const { return ptr == other.getDereferencablePtr(); }
	// template<class T1> 
	// bool operator == (const soft_ptr_impl<T1>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_impl<T>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	bool operator != (const soft_ptr_with_zero_offset_impl<T>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }
	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_impl<T1>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_impl<T1>& other ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	// bool operator != (const owning_ptr_impl<T>& other ) const { return ptr != other.t.getTypedPtr(); }
	// template<class T1> 
	// bool operator != (const owning_ptr_impl<T1>& other ) const { return ptr != other.t.getTypedPtr(); }

	// bool operator != (const soft_ptr_impl<T>& other ) const { return ptr != other.getDereferencablePtr(); }
	// template<class T1> 
	// bool operator != (const soft_ptr_impl<T1>& other ) const { return ptr != other.getDereferencablePtr(); }



	bool operator == (std::nullptr_t ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator==(nullptr); }
	bool operator != (std::nullptr_t ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator!=(nullptr); }

	T& operator * () const noexcept { return *get_raw_ptr(); }
	T* operator -> () const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return reinterpret_cast<T*>(ptr); }

	~soft_ptr_with_zero_offset_impl()
	{
		NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR();
//		ptr = nullptr;
	}
};


template<class T>
class soft_ptr_with_zero_offset_no_checks : public soft_ptr_with_zero_offset_base
{
	// template<class TT>
	// friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	template<class TT>
	friend void deallocate_no_checks(soft_ptr_with_zero_offset_no_checks<TT>&);

	template<class TT>
	friend soft_ptr_no_checks<TT> zero_to_soft(const soft_ptr_with_zero_offset_no_checks<TT>&);

	// T* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() { }

	soft_ptr_with_zero_offset_no_checks( make_zero_offset_t, T* raw ) : soft_ptr_with_zero_offset_base(raw) {}
	// {
	// 	ptr = raw;
	// }

	// soft_ptr_with_zero_offset_no_checks( const owning_ptr_no_checks<T>& owner ) :ptr(owner.t) {	}
	// soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner )
	// {
	// 	ptr = owner.t;
	// 	return *this;
	// }


	// soft_ptr_with_zero_offset_no_checks( const owning_ptr_base_no_checks<T>& owner ) :ptr(owner.t) { }
	// soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_base_no_checks<T>& owner )
	// {
	// 	ptr = owner.t;
	// 	return *this;
	// }

	// soft_ptr_with_zero_offset_no_checks( const soft_ptr_no_checks<T>& other ) :ptr(other.t) { }
	// soft_ptr_with_zero_offset_no_checks<T>& operator = ( const soft_ptr_no_checks<T>& other )
	// {
	// 	ptr = other.t;
	// 	return *this;
	// }

	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks<T>& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator = ( const soft_ptr_with_zero_offset_no_checks<T>& ) = default;
	// {
	// 	ptr = other.ptr;
	// 	return *this;
	// }


	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks<T>&& ) = default;
	soft_ptr_with_zero_offset_no_checks<T>& operator = ( soft_ptr_with_zero_offset_no_checks<T>&& ) = default;
	// {
	// 	ptr = other.ptr;
	// 	return *this;
	// }

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t ) {}
	soft_ptr_with_zero_offset_no_checks& operator = ( std::nullptr_t )
		{ soft_ptr_with_zero_offset_base::reset(); return *this; }
	// {
	// 	reset();
	// 	return *this;
	// }

	// void swap( soft_ptr_with_zero_offset_no_checks<T>& other )
	// {
	// 	T* tmp = ptr;
	// 	ptr = other.ptr;
	// 	other.ptr = tmp;
	// }
	using soft_ptr_with_zero_offset_base::swap;

// 	soft_ptr_no_checks<T> get_soft() const
// 	{
// //		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr != nullptr );
// //		FirstControlBlock* cb = getControlBlock_( ptr );
// 		return soft_ptr_no_checks<T>( fbc_ptr_t(), ptr );
// 	}

	// explicit operator bool() const noexcept
	// {
	// 	return ptr != nullptr;
	// }

	// void reset() { ptr = nullptr; }
	using soft_ptr_with_zero_offset_base::operator bool;
	using soft_ptr_with_zero_offset_base::reset;

	// bool operator == (const owning_ptr_no_checks<T>& other ) const { return ptr == other.t; }
	// template<class T1> 
	// bool operator == (const owning_ptr_no_checks<T1>& other ) const { return ptr == other.t; }

	// bool operator == (const soft_ptr_no_checks<T>& other ) const { return ptr == other.getDereferencablePtr(); }
	// template<class T1> 
	// bool operator == (const soft_ptr_no_checks<T1>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_no_checks<T>& other ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	bool operator != (const soft_ptr_with_zero_offset_no_checks<T>& other ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_no_checks<T1>& other ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator==(other); }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_no_checks<T1>& other ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator!=(other); }

	// bool operator != (const owning_ptr_no_checks<T>& other ) const { return ptr != other.t; }
	// template<class T1> 
	// bool operator != (const owning_ptr_no_checks<T1>& other ) const { return ptr != other.t; }

	// bool operator != (const soft_ptr_no_checks<T>& other ) const { return ptr != other.getDereferencablePtr(); }
	// template<class T1> 
	// bool operator != (const soft_ptr_no_checks<T1>& other ) const { return ptr != other.getDereferencablePtr(); }


	bool operator == (std::nullptr_t ) const noexcept
		{ return soft_ptr_with_zero_offset_base::operator==(nullptr); }
	bool operator != (std::nullptr_t ) const noexcept 
		{ return soft_ptr_with_zero_offset_base::operator!=(nullptr); }

	T& operator * () const noexcept { return *get_raw_ptr(); }
	T* operator -> () const noexcept { return get_raw_ptr(); }
	T* get_raw_ptr() const noexcept { return reinterpret_cast<T*>(ptr); }

	~soft_ptr_with_zero_offset_no_checks()
	{
		NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR();
	}
};

template<class T, memory_safety S>
using soft_ptr_with_zero_offset = std::conditional_t<S == memory_safety::safe, 
			soft_ptr_with_zero_offset_impl<T>,
			soft_ptr_with_zero_offset_no_checks<T>>; 

} // namespace safe_memory::detail

#endif // SAFE_MEMORY_DETAIL_SOFT_PTR_WITH_ZERO_OFFSET_H
