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

#ifndef SAFE_PTR_WITH_ZERO_OFFSET_H
#define SAFE_PTR_WITH_ZERO_OFFSET_H

#include "safe_ptr_common.h"
#include "safe_ptr_impl.h"

namespace nodecpp::safememory::lib_helpers
{

template<class T>
class soft_ptr_with_zero_offset_impl
{
	friend class owning_ptr_impl<T>;
	friend struct FirstControlBlock;

	template<class TT>
	friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	T* ptr= nullptr;

	FirstControlBlock* getControlBlock() const { return getControlBlock_( ptr ); }
	static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:

	static constexpr memory_safety is_safe = memory_safety::safe;
 
	soft_ptr_with_zero_offset_impl() {}

	soft_ptr_with_zero_offset_impl( const owning_ptr_impl<T>& owner )
	{
		ptr = owner.t.getTypedPtr();
	}
	soft_ptr_with_zero_offset_impl<T>& operator = ( const owning_ptr_impl<T>& owner )
	{
		ptr = owner.t.getTypedPtr();
		return *this;
	}


	soft_ptr_with_zero_offset_impl( const soft_ptr_with_zero_offset_impl<T>& other )
	{
		ptr = other.ptr;
	}
	soft_ptr_with_zero_offset_impl<T>& operator = ( const soft_ptr_with_zero_offset_impl<T>& other )
	{
		ptr = other.ptr;
		return *this;
	}


	soft_ptr_with_zero_offset_impl( soft_ptr_with_zero_offset_impl<T>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return;
		ptr = other.ptr;
	}

	soft_ptr_with_zero_offset_impl<T>& operator = ( soft_ptr_with_zero_offset_impl<T>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return *this;
		ptr = other.ptr;
		return *this;
	}

	soft_ptr_with_zero_offset_impl( std::nullptr_t nulp ) {}
	soft_ptr_with_zero_offset_impl& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_with_zero_offset_impl<T>& other )
	{
		T* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	soft_ptr_impl<T> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr != nullptr );
		FirstControlBlock* cb = getControlBlock_( ptr );
		return soft_ptr_impl<T>( cb, ptr );
	}

	explicit operator bool() const noexcept
	{
		return ptr != nullptr;
	}

	void reset() { ptr = nullptr; }

	bool operator == (const owning_ptr_impl<T>& other ) const { return ptr == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const { return ptr == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<T>& other ) const { return ptr == other.getDereferencablePtr(); }
	template<class T1> 
	bool operator == (const soft_ptr_impl<T1>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_impl<T>& other ) const { return ptr == other.ptr; }
	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_impl<T1>& other ) const { return ptr == other.ptr; }

	bool operator != (const owning_ptr_impl<T>& other ) const { return ptr != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const { return ptr != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<T>& other ) const { return ptr != other.getDereferencablePtr(); }
	template<class T1> 
	bool operator != (const soft_ptr_impl<T1>& other ) const { return ptr != other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_with_zero_offset_impl<T>& other ) const { return ptr != other.ptr; }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_impl<T1>& other ) const { return ptr != other.ptr; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return ptr != nullptr; }

	T& operator * () const
	{
		return *(this->ptr);
	}

	T* operator -> () const 
	{
		return this->ptr;
	}

	~soft_ptr_with_zero_offset_impl()
	{
		ptr = nullptr;
	}
};


} // namespace nodecpp::safememory::libhelpers

#endif // SAFE_PTR_WITH_ZERO_OFFSET_H
