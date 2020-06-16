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
#include "safe_ptr_no_checks.h"

namespace nodecpp::safememory::lib_helpers
{

template<class T>
class soft_ptr_with_zero_offset_impl
{
	friend class owning_ptr_impl<T>;

	friend struct ::nodecpp::safememory::FirstControlBlock;

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


	soft_ptr_with_zero_offset_impl( const owning_ptr_base_impl<T>& owner )
	{
		ptr = owner.t.getTypedPtr();
	}
	soft_ptr_with_zero_offset_impl<T>& operator = ( const owning_ptr_base_impl<T>& owner )
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

template<class T> bool operator != (const owning_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 != p1; }
template<class T> bool operator == (const owning_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 == p1; }

template<class T> bool operator != (const soft_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 != p1; }
template<class T> bool operator == (const soft_ptr_impl<T>& p1, const soft_ptr_with_zero_offset_impl<T>& p2 ) { return p2 == p1; }

template<class T>
class soft_ptr_with_zero_offset_no_checks
{
	template<class TT>
	friend class soft_ptr_with_zero_offset_base_no_checks;
	template<class TT>
	friend class soft_ptr_with_zero_offset_no_checks;

	T* ptr = nullptr;

public:

	static constexpr memory_safety is_safe = memory_safety::none;
 
	soft_ptr_with_zero_offset_no_checks() {}

	soft_ptr_with_zero_offset_no_checks( const owning_ptr_no_checks<T>& owner )
	{
		ptr = owner.t;
	}
	soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner )
	{
		ptr = owner.t;
		return *this;
	}


	soft_ptr_with_zero_offset_no_checks( const owning_ptr_base_no_checks<T>& owner )
	{
		ptr = owner.t;
	}
	soft_ptr_with_zero_offset_no_checks<T>& operator = ( const owning_ptr_base_no_checks<T>& owner )
	{
		ptr = owner.t;
		return *this;
	}


	soft_ptr_with_zero_offset_no_checks( const soft_ptr_with_zero_offset_no_checks<T>& other )
	{
		ptr = other.ptr;
	}
	soft_ptr_with_zero_offset_no_checks<T>& operator = ( const soft_ptr_with_zero_offset_no_checks<T>& other )
	{
		ptr = other.ptr;
		return *this;
	}


	soft_ptr_with_zero_offset_no_checks( soft_ptr_with_zero_offset_no_checks<T>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return;
		ptr = other.ptr;
	}

	soft_ptr_with_zero_offset_no_checks<T>& operator = ( soft_ptr_with_zero_offset_no_checks<T>&& other )
	{
		// Note: we do not null the 'other': behaves as an ordinary (raw) pointer
		if ( this == &other ) return *this;
		ptr = other.ptr;
		return *this;
	}

	soft_ptr_with_zero_offset_no_checks( std::nullptr_t nulp ) {}
	soft_ptr_with_zero_offset_no_checks& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_with_zero_offset_no_checks<T>& other )
	{
		T* tmp = ptr;
		ptr = other.ptr;
		other.ptr = tmp;
	}

	soft_ptr_no_checks<T> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr != nullptr );
		FirstControlBlock* cb = getControlBlock_( ptr );
		return soft_ptr_no_checks<T>( cb, ptr );
	}

	explicit operator bool() const noexcept
	{
		return ptr != nullptr;
	}

	void reset() { ptr = nullptr; }

	bool operator == (const owning_ptr_no_checks<T>& other ) const { return ptr == other.t; }
	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return ptr == other.t; }

	bool operator == (const soft_ptr_no_checks<T>& other ) const { return ptr == other.getDereferencablePtr(); }
	template<class T1> 
	bool operator == (const soft_ptr_no_checks<T1>& other ) const { return ptr == other.getDereferencablePtr(); }

	bool operator == (const soft_ptr_with_zero_offset_no_checks<T>& other ) const { return ptr == other.ptr; }
	template<class T1>
	bool operator == (const soft_ptr_with_zero_offset_no_checks<T1>& other ) const { return ptr == other.ptr; }

	bool operator != (const owning_ptr_no_checks<T>& other ) const { return ptr != other.t; }
	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return ptr != other.t; }

	bool operator != (const soft_ptr_no_checks<T>& other ) const { return ptr != other.getDereferencablePtr(); }
	template<class T1> 
	bool operator != (const soft_ptr_no_checks<T1>& other ) const { return ptr != other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_with_zero_offset_no_checks<T>& other ) const { return ptr != other.ptr; }
	template<class T1>
	bool operator != (const soft_ptr_with_zero_offset_no_checks<T1>& other ) const { return ptr != other.ptr; }

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

	~soft_ptr_with_zero_offset_no_checks()
	{
		ptr = nullptr;
	}
};

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
NODISCARD owning_ptr_base_impl<_Ty> make_owning_base_impl(_Types&&... _Args)
{
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	uint8_t* dataForObj = data + sizeof(FirstControlBlock) - getPrefixByteCount();
	void* stackTmp = thg_stackPtrForMakeOwningCall;
	thg_stackPtrForMakeOwningCall = dataForObj;
	try {
		_Ty* objPtr = new (dataForObj) _Ty(::std::forward<_Types>(_Args)...);
		owning_ptr_base_impl<_Ty> op(make_owning_t(), (_Ty*)(uintptr_t)(dataForObj));
		thg_stackPtrForMakeOwningCall = stackTmp;
		return op;
	}
	catch (...) {
		thg_stackPtrForMakeOwningCall = stackTmp;
		zombieDeallocate(data);
		throw;
	}
	//return owning_ptr_impl<_Ty>(objPtr);
}

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
NODISCARD owning_ptr_base_no_checks<_Ty> make_owning_base_no_checks(_Types&&... _Args)
{
	uint8_t* data = reinterpret_cast<uint8_t*>( allocate( sizeof(_Ty) ) );
	try {
		new (data) _Ty(::std::forward<_Types>(_Args)...);
		owning_ptr_base_no_checks<_Ty> op( make_owning_t(), (_Ty*)(data) );
		return op;
	}
	catch (...) {
		deallocate(data);
	}
}

template<class T> bool operator != (const owning_ptr_no_checks<T>& p1, const soft_ptr_with_zero_offset_no_checks<T>& p2 ) { return p2 != p1; }
template<class T> bool operator == (const owning_ptr_no_checks<T>& p1, const soft_ptr_with_zero_offset_no_checks<T>& p2 ) { return p2 == p1; }

template<class T> bool operator != (const soft_ptr_no_checks<T>& p1, const soft_ptr_with_zero_offset_no_checks<T>& p2 ) { return p2 != p1; }
template<class T> bool operator == (const soft_ptr_no_checks<T>& p1, const soft_ptr_with_zero_offset_no_checks<T>& p2 ) { return p2 == p1; }


template<class T, memory_safety is_safe> struct soft_ptr_with_zero_offset_type_ { typedef soft_ptr_with_zero_offset_impl<T> type; };
template<class T> struct soft_ptr_with_zero_offset_type_<T, memory_safety::none> { typedef soft_ptr_with_zero_offset_no_checks<T> type; };
template<class T> struct soft_ptr_with_zero_offset_type_<T, memory_safety::safe> { typedef soft_ptr_with_zero_offset_impl<T> type; };
template<class T, memory_safety is_safe = safeness_declarator<T>::is_safe> using soft_ptr_with_zero_offset = typename soft_ptr_with_zero_offset_type_<T, is_safe>::type;

template<class T, memory_safety is_safe> struct owning_ptr_with_manual_delete_type_ { typedef owning_ptr_base_impl<T> type; };
template<class T> struct owning_ptr_with_manual_delete_type_<T, memory_safety::none> { typedef owning_ptr_base_no_checks<T> type; };
template<class T> struct owning_ptr_with_manual_delete_type_<T, memory_safety::safe> { typedef owning_ptr_base_impl<T> type; };
template<class T, memory_safety is_safe = safeness_declarator<T>::is_safe> using owning_ptr_with_manual_delete = typename owning_ptr_with_manual_delete_type_<T, is_safe>::type;

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
NODISCARD owning_ptr_with_manual_delete<_Ty> make_owning_with_manual_delete(_Types&&... _Args)
{
	if constexpr ( safeness_declarator<_Ty>::is_safe == memory_safety::safe )
	{
		static_assert( owning_ptr_with_manual_delete<_Ty>::is_safe == memory_safety::safe );
		return make_owning_base_impl<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
	else
	{
		static_assert( owning_ptr_with_manual_delete<_Ty>::is_safe == memory_safety::none );
		return make_owning_base_no_checks<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
}

template<class _Ty, memory_safety is_safe,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
NODISCARD auto make_owning_with_manual_delete_2(_Types&&... _Args) -> owning_ptr_with_manual_delete<_Ty, is_safe>
{
	if constexpr ( is_safe == memory_safety::safe )
	{
		return make_owning_base_impl<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
	else
	{
		return make_owning_base_no_checks<_Ty, _Types ...>( ::std::forward<_Types>(_Args)... );
	}
}


} // namespace nodecpp::safememory::libhelpers

#endif // SAFE_PTR_WITH_ZERO_OFFSET_H
