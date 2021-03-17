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

#ifndef SAFE_PTR_NO_CHECKS_H
#define SAFE_PTR_NO_CHECKS_H

#include "safe_ptr_common.h"
#include "memory_safety.h"

// forward declaration
namespace safememory {
	namespace detail {
		class soft_ptr_helper;
	}
}

namespace safememory::detail
{

template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration
template<class T> class soft_this_ptr_no_checks; // forward declaration
template<class T> class nullable_ptr_no_checks; // forward declaration
class soft_this_ptr2_no_checks; // forward declaration

struct fbc_ptr_t {};

template<class T>
class owning_ptr_base_no_checks
{
//	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class owning_ptr_no_checks;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	T* t;

public:

	static constexpr memory_safety is_safe = memory_safety::none;

	owning_ptr_base_no_checks( make_owning_t, T* t_ ) // make it private with a friend make_owning()!
	{
		t = t_;
	}
	owning_ptr_base_no_checks() { t = nullptr; }
	owning_ptr_base_no_checks( owning_ptr_base_no_checks<T>& other ) = delete;
	owning_ptr_base_no_checks& operator = ( owning_ptr_base_no_checks<T>& other ) = delete;
	owning_ptr_base_no_checks( owning_ptr_base_no_checks<T>&& other ) {t = other.t; other.t = nullptr; }
	owning_ptr_base_no_checks& operator = ( owning_ptr_base_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t;
		other.t = nullptr;
		return *this;
	}
	template<class T1>
	owning_ptr_base_no_checks( owning_ptr_base_no_checks<T1>&& other )
	{
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
	}
	template<class T1>
	owning_ptr_base_no_checks& operator = ( owning_ptr_base_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
		return *this;
	}
	owning_ptr_base_no_checks( std::nullptr_t nulp )
	{
		t = nullptr;
	}
	owning_ptr_base_no_checks& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}
	void do_delete()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
			deallocate_no_checked( const_cast<void*>((const void*)t), alignof(T) );
#else // NODECPP_MEMORY_SAFETY_ON_DEMAND
			deallocate( const_cast<void*>((const void*)t), alignof(T) );
#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND
		}
	}
	~owning_ptr_base_no_checks()
	{
		// NOTE: if this class is used (for lib-internal purposes), it is assumed that do_delete() is already called
		//       this is (so far) the only difference with owning_pt_impl where do_delete() is made private and is only called from ctor
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t) )
			t->~T();
		t = nullptr;
	}

	void swap( owning_ptr_base_no_checks<T>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	nullable_ptr_no_checks<T> get() const
	{
		nullable_ptr_no_checks<T> ret;
		ret.t = t;
		return ret;
	}

	T& operator * () const
	{
		return *t;
	}

	T* operator -> () const 
	{
		return t;
	}

	bool operator == (const soft_ptr_no_checks<T>& other ) const { return t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_no_checks<T1>& other ) const { return t == other.t; }

	bool operator != (const soft_ptr_no_checks<T>& other ) const { return t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_no_checks<T1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t != nullptr; }

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};


template<class T>
class owning_ptr_no_checks : public owning_ptr_base_no_checks<T>
{
	template<class TT>
	friend class owning_ptr_no_checks;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

public:

	static constexpr memory_safety is_safe = memory_safety::none;

	owning_ptr_no_checks( make_owning_t mo, T* t_ ) : owning_ptr_base_no_checks<T>( mo, t_ ) {}
	owning_ptr_no_checks() : owning_ptr_base_no_checks<T>() {}
	owning_ptr_no_checks( owning_ptr_no_checks<T>& other ) = delete;
	owning_ptr_no_checks( owning_ptr_no_checks<T>&& other ) : owning_ptr_base_no_checks<T>( std::move(other) ) {}
	template<class T1>
	owning_ptr_no_checks( owning_ptr_no_checks<T1>&& other ) : owning_ptr_base_no_checks<T>( std::move(other) ) {}
	owning_ptr_no_checks( std::nullptr_t nulp ) : owning_ptr_base_no_checks<T>( nulp ) {}

	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>& other ) = delete;
	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>&& other ) { 
		if ( this == &other ) return *this;
		owning_ptr_base_no_checks<T>::operator = ( std::move( other ) );
		return *this;
	}
	template<class T1>
	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		owning_ptr_base_no_checks<T>::operator = ( std::move( other ) );
		return *this;
	}
	owning_ptr_no_checks& operator = ( std::nullptr_t nulp )
	{
		owning_ptr_base_no_checks<T>::operator = ( nulp );
		return *this;
	}
	~owning_ptr_no_checks()
	{
		this->do_delete();
	}
};

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
NODISCARD owning_ptr_no_checks<_Ty> make_owning_no_checks(_Types&&... _Args)
{
	static_assert( alignof(_Ty) <= NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT );
	uint8_t* data = reinterpret_cast<uint8_t*>( allocate( sizeof(_Ty), alignof(_Ty) ) );
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ((uintptr_t)data & (alignof(_Ty)-1)) == 0, "indeed, alignof(_Ty) = {} and data = 0x{:x}", alignof(_Ty), (uintptr_t)data );
	try {
		_Ty* objPtr = new (data) _Ty(::std::forward<_Types>(_Args)...);
	}
	catch (...) {
		deallocate( data, alignof(_Ty), 0 );
		throw;
	}
#if NODECPP_MEMORY_SAFETY == 0
	owning_ptr_no_checks<_Ty> op( make_owning_t(0), (_Ty*)(data) );
#else
	owning_ptr_no_checks<_Ty> op( make_owning_t(), (_Ty*)(data) );
#endif // NODECPP_MEMORY_SAFETY == 0
	return op;
}



template<class T>
class soft_ptr_base_no_checks
{
	friend class owning_ptr_base_no_checks<T>;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_static_cast_no_checks( soft_ptr_no_checks<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_reinterpret_cast_no_checks( soft_ptr_no_checks<TT1> );

	friend class safememory::detail::soft_ptr_helper;

	T* t;

	soft_ptr_base_no_checks(fbc_ptr_t, T* t_) { t = t_; } // to be used for only types annotaded as [[nodecpp::owning_only]]

public:

	static constexpr memory_safety is_safe = memory_safety::none;

	soft_ptr_base_no_checks() {}

	template<class T1>
	soft_ptr_base_no_checks( const owning_ptr_no_checks<T1>& owner ) { t = owner.t; }
	template<class T1>
	soft_ptr_base_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner ) { t = owner.t; return *this; }


	template<class T1>
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T1>& other ) { t = other.t; }
	template<class T1>
	soft_ptr_base_no_checks<T>& operator = ( const soft_ptr_base_no_checks<T1>& other ) { t = other.t; return *this; }
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T>& other ) { t = other.t; }
	soft_ptr_base_no_checks<T>& operator = ( const soft_ptr_base_no_checks<T>& other ) { t = other.t; return *this; }

	soft_ptr_base_no_checks( const soft_ptr_base_impl<T>& other ) { t = other.getDereferencablePtr(); }
	soft_ptr_base_no_checks<T>& operator = ( const soft_ptr_base_impl<T>& other ) { t = other.getDereferencablePtr(); return *this; }

	soft_ptr_base_no_checks( soft_ptr_base_no_checks<T>&& other ) { t = other.t; other.t = nullptr; }

	soft_ptr_base_no_checks<T>& operator = ( soft_ptr_base_no_checks<T>&& other ) { if ( this == &other ) return *this; t = other.t; other.t = nullptr; return *this; }

	template<class T1>
	soft_ptr_base_no_checks( const owning_ptr_no_checks<T1>& owner, T* t_ ) { t = t_; }

	template<class T1>
	soft_ptr_base_no_checks( const owning_ptr_impl<T1>& owner, T* t_ ) { t = t_; }

	template<class T1>
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T1>& other, T* t_ ) { t = t_; }

	template<class T1>
	soft_ptr_base_no_checks( const soft_ptr_base_impl<T1>& other, T* t_ ) { t = t_; }

	//soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T>& other, T* t_ ) {t = t_; }

	soft_ptr_base_no_checks( std::nullptr_t nulp ) {}
	soft_ptr_base_no_checks& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_base_no_checks<T>& other )
	{
		T* tmp = other.t;
		other.t = t;
		t = tmp;
	}

	nullable_ptr_no_checks<T> get() const
	{
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, t != nullptr );
		nullable_ptr_no_checks<T> ret;
		ret.t = t;
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}

	void reset()
	{
		t = nullptr;
	}

	bool operator == (const owning_ptr_no_checks<T>& other ) const { return t == other.t; }
	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return t == other.t; }

	bool operator == (const soft_ptr_base_no_checks<T>& other ) const { return t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_base_no_checks<T1>& other ) const { return t == other.t; }

	bool operator != (const owning_ptr_no_checks<T>& other ) const { return t != other.t; }
	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return t != other.t; }

	bool operator != (const soft_ptr_base_no_checks<T>& other ) const { return t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_base_no_checks<T1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t != nullptr; }

	~soft_ptr_base_no_checks() { NODECPP_DEBUG_COUNT_SOFT_PTR_BASE_DTOR(); }
};

template<class T>
soft_ptr_no_checks<T> soft_ptr_in_constructor_no_checks(T* ptr) {
	return soft_ptr_no_checks<T>( fbc_ptr_t(), ptr );
}

template<class T>
class soft_ptr_no_checks : public soft_ptr_base_no_checks<T>
{
	friend class owning_ptr_base_no_checks<T>;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_static_cast_no_checks( soft_ptr_no_checks<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_reinterpret_cast_no_checks( soft_ptr_no_checks<TT1> );


private:
	friend class soft_this_ptr_no_checks<T>;
	template<class TT>
	friend class soft_this_ptr_no_checks;
	friend class soft_this_ptr2_no_checks;
	template<class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_in_constructor_no_checks(TT*);
	friend soft_ptr_no_checks<T> soft_ptr_in_constructor_no_checks<>(T*);

	friend class safememory::detail::soft_ptr_helper;

	soft_ptr_no_checks(fbc_ptr_t cb, T* t) : soft_ptr_base_no_checks<T>(cb, t) {} // to be used for only types annotaded as [[nodecpp::owning_only]]

public:
	soft_ptr_no_checks() : soft_ptr_base_no_checks<T>() { this->t = nullptr; }

	template<class T1>
	soft_ptr_no_checks( const owning_ptr_no_checks<T1>& owner ) : soft_ptr_base_no_checks<T>(owner) {}
	soft_ptr_no_checks( const owning_ptr_no_checks<T>& owner ) { this->t = owner.t; }
	//soft_ptr_no_checks( const owning_ptr_impl<T>& owner ) { this->t = owner.t; }
	template<class T1>
	soft_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner )
	{
		soft_ptr_base_no_checks<T>::operator = (owner);
		return *this;
	}
	soft_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner ) { this->t = owner.t; return *this; }
	//soft_ptr_no_checks<T>& operator = ( const owning_ptr_impl<T>& owner ) { this->t = owner.t; return *this; }


	template<class T1>
	soft_ptr_no_checks( const soft_ptr_no_checks<T1>& other ) : soft_ptr_base_no_checks<T>(other) {}
	template<class T1>
	soft_ptr_no_checks<T>& operator = ( const soft_ptr_no_checks<T1>& other )
	{
		soft_ptr_base_no_checks<T>::operator = (other);
		return *this;
	}
	soft_ptr_no_checks( const soft_ptr_no_checks<T>& other ) : soft_ptr_base_no_checks<T>(other) {}
	soft_ptr_no_checks<T>& operator = ( const soft_ptr_no_checks<T>& other )
	{
		soft_ptr_base_no_checks<T>::operator = (other);
		return *this;
	}

	soft_ptr_no_checks( const soft_ptr_impl<T>& other ) : soft_ptr_base_no_checks<T>(other) {}
	soft_ptr_no_checks<T>& operator = ( const soft_ptr_impl<T>& other )
	{
		soft_ptr_base_no_checks<T>::operator = (other);
		return *this;
	}

	soft_ptr_no_checks( soft_ptr_no_checks<T>&& other ) : soft_ptr_base_no_checks<T>( std::move(other) ) {}

	soft_ptr_no_checks<T>& operator = ( soft_ptr_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		soft_ptr_base_no_checks<T>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr_no_checks( const owning_ptr_no_checks<T1>& owner, T* t_ ) : soft_ptr_base_no_checks<T>(owner, t_) {}
	soft_ptr_no_checks( const owning_ptr_no_checks<T>& owner, T* t_ ) { this->t = t_; }

	template<class T1>
	soft_ptr_no_checks( const owning_ptr_impl<T1>& owner, T* t_ ) : soft_ptr_base_no_checks<T>(owner, t_) {}
	soft_ptr_no_checks( const owning_ptr_impl<T>& owner, T* t_ ) { this->t = t_; }

	template<class T1>
	soft_ptr_no_checks( const soft_ptr_no_checks<T1>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}
	soft_ptr_no_checks( const soft_ptr_no_checks<T>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}

	template<class T1>
	soft_ptr_no_checks( const soft_ptr_impl<T1>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}
	soft_ptr_no_checks( const soft_ptr_impl<T>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}

	soft_ptr_no_checks( std::nullptr_t nulp ) : soft_ptr_base_no_checks<T>( nulp ) { this->t = nullptr; }
	soft_ptr_no_checks& operator = ( std::nullptr_t nulp )
	{
		soft_ptr_base_no_checks<T>::operator = (nulp);
		return *this;
	}

	void swap( soft_ptr_no_checks<T>& other )
	{
		soft_ptr_base_no_checks<T>::swap(other);
	}

	nullable_ptr_no_checks<T> get() const
	{
		return soft_ptr_base_no_checks<T>::get();
	}

	T& operator * () const
	{
		//checkNotNullAllSizes( this->t );
		return *(this->t);
	}

	T* operator -> () const 
	{
		//checkNotNullLargeSize( this->t );
		return this->t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	void reset()
	{
		soft_ptr_base_no_checks<T>::reset();
	}

	bool operator == (const owning_ptr_no_checks<T>& other ) const { return this->t == other.t; }
	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	bool operator == (const soft_ptr_no_checks<T>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	bool operator != (const owning_ptr_no_checks<T>& other ) const { return this->t != other.t; }
	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator != (const soft_ptr_no_checks<T>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t != nullptr; }
};

template<>
class soft_ptr_no_checks<void> : public soft_ptr_base_no_checks<void>
{
	template<class TT>
	friend class owning_ptr_base_no_checks;
	friend class owning_ptr_base_no_checks<void>;
	template<class TT>
	friend class owning_ptr_no_checks;
//	template<class TT>
//	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_static_cast_no_checks( soft_ptr_no_checks<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_no_checks<TT> soft_ptr_reinterpret_cast_no_checks( soft_ptr_no_checks<TT1> );

public:
	soft_ptr_no_checks() : soft_ptr_base_no_checks<void>() { t = nullptr; }

	template<class T1>
	soft_ptr_no_checks( const owning_ptr_no_checks<T1>& owner ) : soft_ptr_base_no_checks<void>(owner) {}
	template<class T1>
	soft_ptr_no_checks<void>& operator = ( const owning_ptr_no_checks<T1>& owner )
	{
		soft_ptr_base_no_checks<void>::operator = (owner);
		return *this;
	}


	template<class T1>
	soft_ptr_no_checks( const soft_ptr_no_checks<T1>& other ) : soft_ptr_base_no_checks<void>( other ) {}
	template<class T1>
	soft_ptr_no_checks<void>& operator = ( const soft_ptr_no_checks<T1>& other )
	{
		soft_ptr_base_no_checks<void>::operator = (other);
		return *this;
	}
	soft_ptr_no_checks( const soft_ptr_no_checks<void>& other ) : soft_ptr_base_no_checks<void>( other ) {}
	soft_ptr_no_checks<void>& operator = ( soft_ptr_no_checks<void>& other )
	{
		soft_ptr_base_no_checks<void>::operator = (other);
		return *this;
	}


	soft_ptr_no_checks( soft_ptr_no_checks<void>&& other ) : soft_ptr_base_no_checks<void>(  std::move(other)  ) {}

	soft_ptr_no_checks<void>& operator = ( soft_ptr_no_checks<void>&& other )
	{
		if ( this == &other ) return *this;
		soft_ptr_base_no_checks<void>::operator = ( std::move(other) );
		return *this;
	}

	soft_ptr_no_checks( std::nullptr_t nulp ) : soft_ptr_base_no_checks<void>( nulp ) { this->t = nullptr; }
	soft_ptr_no_checks& operator = ( std::nullptr_t nulp )
	{
		soft_ptr_base_no_checks<void>::operator = (nulp);
		return *this;
	}

	void swap( soft_ptr_no_checks<void>& other )
	{
		soft_ptr_base_no_checks<void>::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	bool operator == (const soft_ptr_no_checks<void>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator != (const soft_ptr_no_checks<void>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t != nullptr; }

	void reset()
	{
		soft_ptr_base_no_checks<void>::reset();
	}
};

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_static_cast_no_checks( soft_ptr_no_checks<T1> p ) {
	soft_ptr_no_checks<T> ret(p,static_cast<T*>(p.t));
	return ret;
}

template<class T, class T1>
soft_ptr_no_checks<T> soft_ptr_reinterpret_cast_no_checks( soft_ptr_no_checks<T1> p ) {
	soft_ptr_no_checks<T> ret(p,reinterpret_cast<T*>(p.t));
	return ret;
}

template<class T>
class soft_this_ptr_no_checks
{
public:

	static constexpr memory_safety is_safe = memory_safety::none;

	soft_this_ptr_no_checks() {}

	soft_this_ptr_no_checks( soft_this_ptr_no_checks& other ) {}
	soft_this_ptr_no_checks& operator = ( soft_this_ptr_no_checks& other ) { return *this; }

	soft_this_ptr_no_checks( soft_this_ptr_no_checks&& other ) {}
	soft_this_ptr_no_checks& operator = ( soft_this_ptr_no_checks&& other ) { return *this; }

	explicit operator bool() const noexcept
	{
		return true;
	}

	template<class TT>
	soft_ptr_no_checks<TT> getSoftPtr(TT* ptr)
	{
		return soft_ptr_no_checks<TT>( fbc_ptr_t(), ptr );
	}

	~soft_this_ptr_no_checks()
	{
	}
};

class soft_this_ptr2_no_checks
{
public:

	static constexpr memory_safety is_safe = memory_safety::none;

	soft_this_ptr2_no_checks() = default;

	soft_this_ptr2_no_checks(soft_this_ptr2_no_checks&) = default;
	soft_this_ptr2_no_checks(soft_this_ptr2_no_checks&&) = default;

	soft_this_ptr2_no_checks& operator=(soft_this_ptr2_no_checks&) = default;
	soft_this_ptr2_no_checks& operator=(soft_this_ptr2_no_checks&&) = default;

	~soft_this_ptr2_no_checks() = default;

	explicit constexpr operator bool() const noexcept { return true; }

	template<class T>
	soft_ptr_no_checks<T> getSoftPtr(T* ptr) const {
		return {fbc_ptr_t(), ptr};
	}
};


template<class T>
class nullable_ptr_base_no_checks
{
	friend class owning_ptr_base_no_checks<T>;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class owning_ptr_no_checks;
	friend class soft_ptr_base_no_checks<T>;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class nullable_ptr_base_no_checks;
	friend class nullable_ptr_no_checks<T>;
	template<class TT>
	friend class nullable_ptr_no_checks;

	template<class T1, class T2>
	friend T1* nullable_cast_no_checks( nullable_ptr_no_checks<T2> p );
	template<class T1>
	friend T1* nullable_cast_no_checks( nullable_ptr_no_checks<T1> p );

	T* t;

	T* get_() const { return t; }

public:

	static constexpr memory_safety is_safe = memory_safety::none;

	nullable_ptr_base_no_checks() { t = nullptr; }
	nullable_ptr_base_no_checks(T* t_) { t = t_; }

	template<class T1>
	nullable_ptr_base_no_checks( const owning_ptr_no_checks<T1>& owner ) { *this = owner.get(); }

	template<class T1>
	nullable_ptr_base_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_base_no_checks( const soft_ptr_no_checks<T1>& other ) { *this = other.get(); }
	nullable_ptr_base_no_checks( const soft_ptr_no_checks<T>& other ) { *this = other.get(); }
	template<class T1>
	nullable_ptr_base_no_checks<T>& operator = ( const soft_ptr_no_checks<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_base_no_checks<T>& operator = ( const soft_ptr_no_checks<T>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_base_no_checks( const nullable_ptr_base_no_checks<T1>& other ) { t = other.t; }
	template<class T1>
	nullable_ptr_base_no_checks<T>& operator = ( const nullable_ptr_base_no_checks<T1>& other ) { t = other.t; return *this; }
	nullable_ptr_base_no_checks( const nullable_ptr_base_no_checks<T>& other ) = default;
	nullable_ptr_base_no_checks<T>& operator = ( nullable_ptr_base_no_checks<T>& other ) = default;

	nullable_ptr_base_no_checks( nullable_ptr_base_no_checks<T>&& other ) = default;
	nullable_ptr_base_no_checks<T>& operator = ( nullable_ptr_base_no_checks<T>&& other ) = default;

	nullable_ptr_base_no_checks( std::nullptr_t nulp ) { t = nullptr; }
	nullable_ptr_base_no_checks& operator = ( std::nullptr_t nulp ) { t = nullptr; return *this; }

	void swap( nullable_ptr_base_no_checks<T>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}


	bool operator == ( const nullable_ptr_base_no_checks<T>& other ) const { return t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_base_no_checks<T1>& other ) const { return t == other.t; }

	bool operator != ( const nullable_ptr_base_no_checks<T>& other ) const { return t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_base_no_checks<T1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return t != nullptr; }

	~nullable_ptr_base_no_checks()
	{
		t = nullptr;
	}
};


template<class T>
class nullable_ptr_no_checks : public nullable_ptr_base_no_checks<T>
{
	friend class owning_ptr_base_no_checks<T>;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	friend class owning_ptr_no_checks<T>;
	friend class soft_ptr_base_no_checks<T>;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class owning_ptr_no_checks;

	template<class T1, class T2>
	friend T1* nullable_cast_no_checks( nullable_ptr_no_checks<T2> p );
	template<class T1>
	friend T1* nullable_cast_no_checks( nullable_ptr_no_checks<T1> p );

public:
	nullable_ptr_no_checks() : nullable_ptr_base_no_checks<T>() {}

	nullable_ptr_no_checks(T* t_) : nullable_ptr_base_no_checks<T>(t_) {}

	template<class T1>
	nullable_ptr_no_checks<T>( const owning_ptr_no_checks<T1>& owner ) : nullable_ptr_base_no_checks<T>(owner) {}
	nullable_ptr_no_checks<T>( const owning_ptr_no_checks<T>& owner ) : nullable_ptr_base_no_checks<T>() {*this = owner.get();}
	template<class T1>
	nullable_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner ) { *this = owner.get(); return *this; }
	nullable_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_no_checks<T>( const soft_ptr_no_checks<T1>& other ) : nullable_ptr_base_no_checks<T>(other) {}
	nullable_ptr_no_checks<T>( const soft_ptr_no_checks<T>& other ) : nullable_ptr_base_no_checks<T>(other) {}
	template<class T1>
	nullable_ptr_no_checks<T>& operator = ( const soft_ptr_no_checks<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_no_checks<T>& operator = ( const soft_ptr_no_checks<T>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_no_checks<T>( const nullable_ptr_no_checks<T1>& other ) : nullable_ptr_base_no_checks<T>(other) {}
	template<class T1>
	nullable_ptr_no_checks<T>& operator = ( const nullable_ptr_no_checks<T1>& other ) { this->t = other.t; return *this; }
	nullable_ptr_no_checks<T>( const nullable_ptr_no_checks<T>& other ) = default;
	nullable_ptr_no_checks<T>& operator = ( nullable_ptr_no_checks<T>& other ) = default;

	nullable_ptr_no_checks<T>( nullable_ptr_no_checks<T>&& other ) = default;
	nullable_ptr_no_checks<T>& operator = ( nullable_ptr_no_checks<T>&& other ) = default;

	nullable_ptr_no_checks( std::nullptr_t nulp ) : nullable_ptr_base_no_checks<T>(nulp) {}
	nullable_ptr_no_checks& operator = ( std::nullptr_t nulp ) { nullable_ptr_base_no_checks<T>::operator = (nulp); return *this; }

	void swap( nullable_ptr_no_checks<T>& other )
	{
		nullable_ptr_base_no_checks<T>::swap( other );
	}

	T& operator * () const
	{
		return *(this->t);
	}

	T* operator -> () const 
	{
		return this->t;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_no_checks<T>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_no_checks<T>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_no_checks()
	{
		this->t = nullptr;
	}
};


template<>
class nullable_ptr_no_checks<void> : public nullable_ptr_base_no_checks<void>
{
	friend class owning_ptr_base_no_checks<void>;
	template<class TT>
	friend class owning_ptr_base_no_checks;
	template<class TT>
	friend class owning_ptr_no_checks;
	friend class soft_ptr_base_no_checks<void>;
	template<class TT>
	friend class soft_ptr_base_no_checks;

	template<class T1, class T2>
	friend T1* nullable_cast_no_checks( nullable_ptr_no_checks<T2> p );
	friend void* nullable_cast_no_checks( nullable_ptr_no_checks<void> p );

public:
	nullable_ptr_no_checks() : nullable_ptr_base_no_checks() {}

	template<class T1>
	nullable_ptr_no_checks(T1& t_) : nullable_ptr_base_no_checks<void>(t_) {}

	template<class T1>
	nullable_ptr_no_checks( const owning_ptr_no_checks<T1>& owner ) : nullable_ptr_base_no_checks(owner) {}
	template<class T1>
	nullable_ptr_no_checks<void>& operator = ( const owning_ptr_no_checks<T1>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_no_checks( const soft_ptr_no_checks<T1>& other ) : nullable_ptr_base_no_checks(other) {}
	nullable_ptr_no_checks( const soft_ptr_no_checks<void>& other ) : nullable_ptr_base_no_checks(other) {}
	template<class T1>
	nullable_ptr_no_checks<void>& operator = ( const soft_ptr_no_checks<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_no_checks<void>& operator = ( const soft_ptr_no_checks<void>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_no_checks( const nullable_ptr_no_checks<T1>& other ) : nullable_ptr_base_no_checks(other) {}
	template<class T1>
	nullable_ptr_no_checks<void>& operator = ( const nullable_ptr_no_checks<T1>& other ) { t = other.t; return *this; }
	nullable_ptr_no_checks( const nullable_ptr_no_checks<void>& other ) = default;
	nullable_ptr_no_checks<void>& operator = ( nullable_ptr_no_checks<void>& other ) = default;

	nullable_ptr_no_checks( nullable_ptr_no_checks<void>&& other ) = default;
	nullable_ptr_no_checks<void>& operator = ( nullable_ptr_no_checks<void>&& other ) = default;

	nullable_ptr_no_checks( std::nullptr_t nulp ) : nullable_ptr_base_no_checks<void>(nulp) {}
	nullable_ptr_no_checks& operator = ( std::nullptr_t nulp ) { nullable_ptr_base_no_checks<void>::operator = (nulp); return *this; }

	void swap( nullable_ptr_no_checks<void>& other )
	{
		nullable_ptr_base_no_checks<void>::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_no_checks<void>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_no_checks<void>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_no_checks()
	{
		this->t = nullptr;
	}
};


template<class T, class T1>
T* nullable_cast_no_checks( nullable_ptr_no_checks<T1> p ) {
	T* ret = p.get_();
	return ret;
}

template<class T>
T* nullable_cast_no_checks( nullable_ptr_no_checks<T> p ) {
	T* ret = p.get_();
	return ret;
}


template<class T, class T1>
nullable_ptr_no_checks<T1> nullable_cast_no_checks( T* p ) {
	return nullable_ptr_no_checks<T1>( p );
}

template<class T>
nullable_ptr_no_checks<T> nullable_cast_no_checks( T* p ) {
	return nullable_ptr_no_checks<T>( p );
}

} // namespace safememory::detail


#endif // SAFE_PTR_NO_CHECKS_H
