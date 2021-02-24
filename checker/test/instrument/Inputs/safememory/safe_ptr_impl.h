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

#ifndef SAFE_PTR_IMPL_H
#define SAFE_PTR_IMPL_H

// #include "safe_ptr_common.h"
#include "memory_safety.h"
// #include "../include/nodecpp_error/nodecpp_error.h"
// #include "safe_memory_error.h"
// #ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO
// #include <stack_info.h>
// #endif // NODECPP_MEMORY_SAFETY_DBG_ADD_PTR_LIFECYCLE_INFO

// forward declaration
namespace safememory {
	namespace detail {
		class soft_ptr_helper;
	}
}

namespace safememory::detail
{
	
// extern thread_local void* thg_stackPtrForMakeOwningCall;

template<class T>
void checkNotNullLargeSize( T* ptr )
{
}

inline
void checkNotNullLargeSize( void* )
{
}

template<class T>
void checkNotNullAllSizes( T* ptr )
{
}

[[noreturn]] inline
void throwPointerOutOfRange()
{
	// TODO: actual implementation
//	throw std::bad_alloc();
		throw "out_of_range";
}


template<class T> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_impl; // forward declaration
template<class T> class nullable_ptr_base_impl; // forward declaration
template<class T> class nullable_ptr_impl; // forward declaration
template<class T> class soft_this_ptr_impl; // forward declaration
class soft_this_ptr2_impl; // forward declaration

struct FirstControlBlock // not reallocatable
{
};
//static_assert( sizeof(FirstControlBlock) == 32 );



inline
FirstControlBlock* getControlBlock_(const void* t) { return reinterpret_cast<FirstControlBlock*>(const_cast<void*>(t)) - 1; }
inline
uint8_t* getAllocatedBlock_(const void* t) { return reinterpret_cast<uint8_t*>(getControlBlock_(t)) + 0; }
inline
uint8_t* getAllocatedBlockFromControlBlock_(void* cb) { return reinterpret_cast<uint8_t*>(cb) + 0; }
inline
void* getPtrToAllocatedObjectFromControlBlock_( void* allocObjPtr ) { return (reinterpret_cast<FirstControlBlock*>(allocObjPtr)) + 1; }


// struct make_owning_t {};

template<class T>
class owning_ptr_base_impl
{
	template<class TT>
	friend class owning_ptr_base_impl;
	friend class owning_ptr_impl<T>;
	template<class TT>
	friend class owning_ptr_impl;

	template<class TT>
	friend class soft_ptr_base_impl;
	template<class TT>
	friend class soft_ptr_impl;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	template<class TT>
	friend void killUnderconsructedOP( owning_ptr_base_impl<TT>& );


public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	// owning_ptr_base_impl( make_owning_t, T* t_ );
	owning_ptr_base_impl();
	owning_ptr_base_impl( const owning_ptr_base_impl<T>& other ) = delete;
	owning_ptr_base_impl& operator = ( const owning_ptr_base_impl<T>& other ) = delete;
	owning_ptr_base_impl( owning_ptr_base_impl<T>&& other );

	owning_ptr_base_impl& operator = ( owning_ptr_base_impl<T>&& other );
	template<class T1>
	owning_ptr_base_impl( owning_ptr_base_impl<T1>&& other );
	template<class T1>
	owning_ptr_base_impl& operator = ( owning_ptr_base_impl<T>&& other );
	owning_ptr_base_impl( std::nullptr_t nulp );
	owning_ptr_base_impl& operator = ( std::nullptr_t nulp );
	
	~owning_ptr_base_impl();

	void do_delete();

	void reset();
	void swap( owning_ptr_base_impl<T>& other );
	nullable_ptr_impl<T> get() const;
	T& operator * () const;
	T* operator -> () const;
	bool operator == (const soft_ptr_impl<T>& other ) const;
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const;

	bool operator != (const soft_ptr_impl<T>& other ) const;
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const;

	bool operator == (std::nullptr_t nullp ) const;
	bool operator != (std::nullptr_t nullp ) const;

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept;
};

template<class T>
class owning_ptr_impl : public owning_ptr_base_impl<T>
{
	template<class TT>
	friend class owning_ptr_impl;
	template<class TT>
	friend class soft_ptr_base_impl;
	template<class TT>
	friend class soft_ptr_impl;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	using owning_ptr_base_impl<T>::do_delete;

public:
	static constexpr memory_safety is_safe = memory_safety::safe;

	// owning_ptr_impl( make_owning_t mo, T* t_ ) : owning_ptr_base_impl<T>( mo, t_ ) {}
	owning_ptr_impl() : owning_ptr_base_impl<T>() {}
	owning_ptr_impl( const owning_ptr_impl<T>& other ) = delete;
	owning_ptr_impl( owning_ptr_impl<T>&& other ) : owning_ptr_base_impl<T>( std::move(other) ) {}
	template<class T1>
	owning_ptr_impl( owning_ptr_impl<T1>&& other ) : owning_ptr_base_impl<T>( std::move(other) ) {}
	owning_ptr_impl( std::nullptr_t nulp ) : owning_ptr_base_impl<T>( nulp ) {}

	owning_ptr_impl& operator = ( const owning_ptr_impl<T>& other ) = delete;
	owning_ptr_impl& operator = ( owning_ptr_impl<T>&& other ) { 
		if ( this == &other ) return *this;
		owning_ptr_base_impl<T>::operator = ( std::move( other ) );
		return *this;
	}
	template<class T1>
	owning_ptr_impl& operator = ( owning_ptr_impl<T>&& other )
	{
		if ( this == &other ) return *this;
		owning_ptr_base_impl<T>::operator = ( std::move( other ) );
		return *this;
	}
	owning_ptr_impl& operator = ( std::nullptr_t nulp )
	{
		owning_ptr_base_impl<T>::operator = ( nulp );
		return *this;
	}
	~owning_ptr_impl()
	{
		this->do_delete();
	}
};

template<class T>
void killUnderconsructedOP( owning_ptr_base_impl<T>& p );

template<class _Ty,
	class... _Types>
owning_ptr_impl<_Ty> make_owning_impl(_Types&&... _Args)
{
	return {};
}



template<class T>
class soft_ptr_base_impl
{
	friend class owning_ptr_base_impl<T>;
	friend class owning_ptr_impl<T>;
	template<class TT>
	friend class soft_ptr_base_impl;
	template<class TT>
	friend class soft_ptr_impl;

	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1> );
	friend struct FirstControlBlock;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	friend struct FirstControlBlock;

	friend class safememory::detail::soft_ptr_helper;



	soft_ptr_base_impl(FirstControlBlock* cb, T* t);

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	void dbgCheckMySlotConsistency() const {}

	soft_ptr_base_impl() {}

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_base_impl<T1>& owner );
	template<class T1>
	soft_ptr_base_impl<T>& operator = ( const owning_ptr_base_impl<T1>& owner );

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_impl<T1>& owner );
	template<class T1>
	soft_ptr_base_impl<T>& operator = ( const owning_ptr_impl<T1>& owner );


	template<class T1>
	soft_ptr_base_impl( const soft_ptr_base_impl<T1>& other );
	template<class T1>
	soft_ptr_base_impl<T>& operator = ( const soft_ptr_base_impl<T1>& other );
	soft_ptr_base_impl( const soft_ptr_base_impl<T>& other );
	soft_ptr_base_impl<T>& operator = ( const soft_ptr_base_impl<T>& other );
	soft_ptr_base_impl( soft_ptr_base_impl<T>&& other );

	soft_ptr_base_impl<T>& operator = ( soft_ptr_base_impl<T>&& other );

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_base_impl<T1>& owner, T* t_ );

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_impl<T1>& owner, T* t_ );

	template<class T1>
	soft_ptr_base_impl( const soft_ptr_base_impl<T1>& other, T* t_ );
	soft_ptr_base_impl( const soft_ptr_base_impl<T>& other, T* t_ );

	soft_ptr_base_impl( std::nullptr_t nulp );

	void swap( soft_ptr_base_impl<T>& other );

	nullable_ptr_impl<T> get() const;

	explicit operator bool() const noexcept;
	void reset();

	bool operator == (const owning_ptr_base_impl<T>& other ) const;
	bool operator == (const owning_ptr_impl<T>& other ) const;
	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const;

	bool operator == (const soft_ptr_base_impl<T>& other ) const;
	template<class T1>
	bool operator == (const soft_ptr_base_impl<T1>& other ) const;

	bool operator != (const owning_ptr_base_impl<T>& other ) const;
	bool operator != (const owning_ptr_impl<T>& other ) const;
	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const;

	bool operator != (const soft_ptr_base_impl<T>& other ) const;
	template<class T1>
	bool operator != (const soft_ptr_base_impl<T1>& other ) const;

	bool operator == (std::nullptr_t nullp ) const;
	bool operator != (std::nullptr_t nullp ) const;

	~soft_ptr_base_impl();
};

template<class T>
soft_ptr_impl<T> soft_ptr_in_constructor_impl(T* ptr) {
	return {};
}

template<class T>
class soft_ptr_impl : public soft_ptr_base_impl<T>
{
	friend class owning_ptr_base_impl<T>;
	friend class owning_ptr_impl<T>;
	template<class TT>
	friend class soft_ptr_impl;
	template<class TT>
	friend class soft_ptr_base_impl;

	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1> );
	friend struct FirstControlBlock;

private:
	friend class soft_this_ptr_impl<T>;
	template<class TT>
	friend class soft_this_ptr_impl;
	friend class soft_this_ptr2_impl;
	template<class TT>
	friend soft_ptr_impl<TT> soft_ptr_in_constructor_impl(TT* ptr);
	friend soft_ptr_impl<T> soft_ptr_in_constructor_impl<>(T* ptr);

	friend class safememory::detail::soft_ptr_helper;

	soft_ptr_impl(FirstControlBlock* cb, T* t) : soft_ptr_base_impl<T>(cb, t) {} // to be used for only types annotaded as [[nodecpp::owning_only]]

public:
	soft_ptr_impl();

	template<class T1>
	soft_ptr_impl( const owning_ptr_base_impl<T1>& owner ) : soft_ptr_base_impl<T>(owner) {}
	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1>& owner ) : soft_ptr_base_impl<T>(owner) {}

	soft_ptr_impl( const owning_ptr_impl<T>& owner );
	template<class T1>
	soft_ptr_impl<T>& operator = ( const owning_ptr_base_impl<T1>& owner );
	template<class T1>
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T1>& owner );

	soft_ptr_impl<T>& operator = ( const owning_ptr_base_impl<T>& owner );
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T>& owner );


	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1>& other ) : soft_ptr_base_impl<T>(other) {}
	template<class T1>
	soft_ptr_impl<T>& operator = ( const soft_ptr_impl<T1>& other )
	{
		soft_ptr_base_impl<T>::operator = (other);
		return *this;
	}
	soft_ptr_impl( const soft_ptr_impl<T>& other ) : soft_ptr_base_impl<T>(other) {}
	soft_ptr_impl<T>& operator = ( const soft_ptr_impl<T>& other )
	{
		soft_ptr_base_impl<T>::operator = (other);
		return *this;
	}


	soft_ptr_impl( soft_ptr_impl<T>&& other ) : soft_ptr_base_impl<T>( std::move(other) ) {}

	soft_ptr_impl<T>& operator = ( soft_ptr_impl<T>&& other )
	{
		if ( this == &other ) return *this;
		soft_ptr_base_impl<T>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr_impl( const owning_ptr_base_impl<T1>& owner, T* t_ ) : soft_ptr_base_impl<T>(owner, t_) {}
	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1>& owner, T* t_ ) : soft_ptr_base_impl<T>(owner, t_) {}

	soft_ptr_impl( const owning_ptr_base_impl<T>& owner, T* t_ );
	soft_ptr_impl( const owning_ptr_impl<T>& owner, T* t_ );

	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1>& other, T* t_ ) : soft_ptr_base_impl<T>(other, t_) {}
	soft_ptr_impl( const soft_ptr_impl<T>& other, T* t_ ) : soft_ptr_base_impl<T>(other, t_) {}

	soft_ptr_impl( std::nullptr_t nulp );
	soft_ptr_impl& operator = ( std::nullptr_t nulp )
	{
		soft_ptr_base_impl<T>::operator = (nulp);
		return *this;
	}

	void swap( soft_ptr_impl<T>& other )
	{
		soft_ptr_base_impl<T>::swap(other);
	}

	nullable_ptr_impl<T> get() const;

	T& operator * () const;

	T* operator -> () const;
	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept;
	void reset()
	{
		soft_ptr_base_impl<T>::reset();
	}

	bool operator == (const owning_ptr_base_impl<T>& other ) const;
	bool operator == (const owning_ptr_impl<T>& other ) const;
	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const;

	bool operator == (const soft_ptr_impl<T>& other ) const;
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const;

	bool operator != (const owning_ptr_base_impl<T>& other ) const;
	bool operator != (const owning_ptr_impl<T>& other ) const;
	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const;

	bool operator != (const soft_ptr_impl<T>& other ) const;
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const;

	bool operator == (std::nullptr_t nullp ) const;
	bool operator != (std::nullptr_t nullp ) const;
};

template<>
class soft_ptr_impl<void> : public soft_ptr_base_impl<void>
{
	template<class TT>
	friend class owning_ptr_base_impl;
	template<class TT>
	friend class owning_ptr_impl;
	template<class TT>
	friend class soft_ptr_impl;

	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1> );
	friend struct FirstControlBlock;

public:
	soft_ptr_impl();


	template<class T1>
	soft_ptr_impl( const owning_ptr_base_impl<T1>& owner ) : soft_ptr_base_impl<void>(owner) {}
	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1>& owner ) : soft_ptr_base_impl<void>(owner) {}

	template<class T1>
	soft_ptr_impl<void>& operator = ( const owning_ptr_base_impl<T1>& owner )
	{
		soft_ptr_base_impl<void>::operator = (owner);
		return *this;
	}
	template<class T1>
	soft_ptr_impl<void>& operator = ( const owning_ptr_impl<T1>& owner )
	{
		soft_ptr_base_impl<void>::operator = (owner);
		return *this;
	}


	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1>& other ) : soft_ptr_base_impl<void>( other ) {}
	template<class T1>
	soft_ptr_impl<void>& operator = ( const soft_ptr_impl<T1>& other )
	{
		soft_ptr_base_impl<void>::operator = (other);
		return *this;
	}

	soft_ptr_impl( std::nullptr_t nulp );
	soft_ptr_impl& operator = ( std::nullptr_t nulp )
	{
		soft_ptr_base_impl<void>::operator = (nulp);
		return *this;
	}
	soft_ptr_impl( const soft_ptr_impl<void>& other ) : soft_ptr_base_impl<void>( other ) {}
	soft_ptr_impl<void>& operator = ( soft_ptr_impl<void>& other )
	{
		soft_ptr_base_impl<void>::operator = (other);
		return *this;
	}


	soft_ptr_impl( soft_ptr_impl<void>&& other ) : soft_ptr_base_impl<void>(  std::move(other)  ) {}

	soft_ptr_impl<void>& operator = ( soft_ptr_impl<void>&& other )
	{
		if ( this == &other ) return *this;
		soft_ptr_base_impl<void>::operator = ( std::move(other) );
		return *this;
	}

	void swap( soft_ptr_impl<void>& other )
	{
		soft_ptr_base_impl<void>::swap( other );
	}

	explicit operator bool() const noexcept;

	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const;

	bool operator == (const soft_ptr_impl<void>& other ) const;
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const;

	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const;
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const;

	bool operator != (const soft_ptr_impl<void>& other ) const;
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const;

	bool operator == (std::nullptr_t nullp ) const;
	bool operator != (std::nullptr_t nullp ) const;

	void reset()
	{
		soft_ptr_base_impl<void>::reset();
	}
};


template<class T, class T1>
soft_ptr_impl<T> soft_ptr_static_cast_impl( soft_ptr_impl<T1> p ) {
	return {};
}

template<class T, class T1>
soft_ptr_impl<T> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<T1> p ) {
	return {};
}


template<class T>
class soft_this_ptr_impl
{
	FirstControlBlock* cbPtr = nullptr;
	uint32_t offset;

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	soft_this_ptr_impl();

	soft_this_ptr_impl( soft_this_ptr_impl& other ) {}
	soft_this_ptr_impl& operator = ( soft_this_ptr_impl& other ) { return *this; }

	soft_this_ptr_impl( soft_this_ptr_impl&& other ) {}
	soft_this_ptr_impl& operator = ( soft_this_ptr_impl&& other ) { return *this; }

	explicit operator bool() const noexcept
	{
		return cbPtr != nullptr;
	}

	template<class TT>
	soft_ptr_impl<TT> getSoftPtr(TT* ptr);

	~soft_this_ptr_impl()
	{
	}
};

class soft_this_ptr2_impl
{
	FirstControlBlock* cbPtr = nullptr;

	static FirstControlBlock* getCbPtr() noexcept {
		return nullptr;
	}

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	soft_this_ptr2_impl() : cbPtr(getCbPtr()) {}
	soft_this_ptr2_impl(const soft_this_ptr2_impl&) : cbPtr(getCbPtr()) {}
	soft_this_ptr2_impl(soft_this_ptr2_impl&&) : cbPtr(getCbPtr()) {}

	soft_this_ptr2_impl& operator=(const soft_this_ptr2_impl&) { return *this; }
	soft_this_ptr2_impl& operator=(soft_this_ptr2_impl&&) { return *this; }

	~soft_this_ptr2_impl() = default;

	explicit operator bool() const noexcept {
		return cbPtr != nullptr;
	}

	template<class T>
	soft_ptr_impl<T> getSoftPtr(T* ptr) const {

		return {};
	}
};

template<class T>
class nullable_ptr_base_impl
{
	friend class owning_ptr_base_impl<T>;
	friend class owning_ptr_impl<T>;
	friend class soft_ptr_base_impl<T>;
	template<class TT>
	friend class soft_ptr_base_impl;
	//friend class soft_ptr_impl<T>;
	template<class TT>
	friend class owning_ptr_base_impl;
	template<class TT>
	friend class owning_ptr_impl;

	template<class TT>
	friend class nullable_ptr_base_impl;
	friend class nullable_ptr_impl<T>;
	template<class TT>
	friend class nullable_ptr_impl;

	template<class T1, class T2>
	friend T1* nullable_cast_impl( nullable_ptr_impl<T2> p );
	template<class T1>
	friend T1* nullable_cast_impl( nullable_ptr_impl<T1> p );

	T* t;

	T* get_() const { 
		checkNotNullAllSizes( this->t );
		return this->t;
	}

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	nullable_ptr_base_impl() { t = nullptr; }
	
	nullable_ptr_base_impl(T* t_) { t = t_; }

	template<class T1>
	nullable_ptr_base_impl( const owning_ptr_impl<T1>& owner ) { *this = owner.get(); }

	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const owning_ptr_impl<T1>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_base_impl( const soft_ptr_impl<T1>& other ) { *this = other.get(); }
	nullable_ptr_base_impl( const soft_ptr_impl<T>& other ) { *this = other.get(); }
	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const soft_ptr_impl<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_base_impl<T>& operator = ( const soft_ptr_impl<T>& other ) { *this = other.get(); return *this; }
	template<class T1>
	nullable_ptr_base_impl( const nullable_ptr_base_impl<T1>& other ) { t = other.t; }
	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const nullable_ptr_base_impl<T1>& other ) { t = other.t; return *this; }
	nullable_ptr_base_impl( const nullable_ptr_base_impl<T>& other ) = default;
	nullable_ptr_base_impl<T>& operator = ( const nullable_ptr_base_impl<T>& other ) = default;

	nullable_ptr_base_impl( nullable_ptr_base_impl<T>&& other ) = default;
	nullable_ptr_base_impl<T>& operator = ( nullable_ptr_base_impl<T>&& other ) = default;

	nullable_ptr_base_impl( std::nullptr_t nulp ) { t = nullptr; }
	nullable_ptr_base_impl& operator = ( std::nullptr_t nulp ) { t = nullptr; return *this; }

	void swap( nullable_ptr_base_impl<T>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}


	bool operator == ( const nullable_ptr_base_impl<T>& other ) const { return t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_base_impl<T1>& other ) const { return t == other.t; }

	bool operator != ( const nullable_ptr_base_impl<T>& other ) const { return t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_base_impl<T1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return t != nullptr; }

	~nullable_ptr_base_impl()
	{
		t = nullptr;
	}
};


template<class T>
class nullable_ptr_impl : public nullable_ptr_base_impl<T>
{
	friend class owning_ptr_impl<T>;
	friend class soft_ptr_base_impl<T>;
	template<class TT>
	friend class soft_ptr_base_impl;
	template<class TT>
	friend class owning_ptr_impl;

	template<class T1, class T2>
	friend T1* nullable_cast_impl( nullable_ptr_impl<T2> p );
	template<class T1>
	friend T1* nullable_cast_impl( nullable_ptr_impl<T1> p );

public:
	nullable_ptr_impl() : nullable_ptr_base_impl<T>() {}

	nullable_ptr_impl(T* t_) : nullable_ptr_base_impl<T>(t_) {}

	template<class T1>
	nullable_ptr_impl( const owning_ptr_impl<T1>& owner ) : nullable_ptr_base_impl<T>(owner) {}
	nullable_ptr_impl( const owning_ptr_impl<T>& owner ) : nullable_ptr_base_impl<T>() {*this = owner.get();}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const owning_ptr_impl<T1>& owner ) { *this = owner.get(); return *this; }
	nullable_ptr_impl<T>& operator = ( const owning_ptr_impl<T>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const soft_ptr_impl<T1>& other ) : nullable_ptr_base_impl<T>(other) {}
	nullable_ptr_impl( const soft_ptr_impl<T>& other ) : nullable_ptr_base_impl<T>(other) {}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const soft_ptr_impl<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_impl<T>& operator = ( const soft_ptr_impl<T>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const nullable_ptr_impl<T1>& other ) : nullable_ptr_base_impl<T>(other) {}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const nullable_ptr_impl<T1>& other ) { this->t = other.t; return *this; }
	nullable_ptr_impl( const nullable_ptr_impl<T>& other ) = default;
	nullable_ptr_impl<T>& operator = ( const nullable_ptr_impl<T>& other ) = default;

	nullable_ptr_impl( nullable_ptr_impl<T>&& other ) = default;
	nullable_ptr_impl<T>& operator = ( nullable_ptr_impl<T>&& other ) = default;

	nullable_ptr_impl( std::nullptr_t nulp ) : nullable_ptr_base_impl<T>(nulp) {}
	nullable_ptr_impl& operator = ( std::nullptr_t nulp ) { nullable_ptr_base_impl<T>::operator = (nulp); 	return *this; }

	void swap( nullable_ptr_impl<T>& other )
	{
		nullable_ptr_base_impl<T>::swap( other );
	}

	T& operator * () const
	{
		checkNotNullAllSizes( this->t );
		return *(this->t);
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( this->t );
		return this->t;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_impl<T>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_impl<T1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_impl<T>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_impl<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_impl()
	{
		this->t = nullptr;
	}
};


template<>
class nullable_ptr_impl<void> : public nullable_ptr_base_impl<void>
{
	template<class TT>
	friend class owning_ptr_impl;
	friend class soft_ptr_base_impl<void>;
	template<class TT>
	friend class soft_ptr_base_impl;

	template<class T1, class T2>
	friend T1* nullable_cast_impl( nullable_ptr_impl<T2> p );
	friend void* nullable_cast_impl( nullable_ptr_impl<void> p );

public:
	nullable_ptr_impl() : nullable_ptr_base_impl<void>() {}

	template<class T1>
	nullable_ptr_impl(T1* t_) : nullable_ptr_base_impl<void>(t_) {}

	template<class T1>
	nullable_ptr_impl( const owning_ptr_impl<T1>& owner ) : nullable_ptr_base_impl(owner) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const owning_ptr_impl<T1>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const soft_ptr_impl<T1>& other ) : nullable_ptr_base_impl(other) {}
	nullable_ptr_impl( const soft_ptr_impl<void>& other ) : nullable_ptr_base_impl(other) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const soft_ptr_impl<T1>& other ) { *this = other.get(); return *this; }
	nullable_ptr_impl<void>& operator = ( const soft_ptr_impl<void>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const nullable_ptr_impl<T1>& other ) : nullable_ptr_base_impl(other) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const nullable_ptr_impl<T1>& other ) { t = other.t; return *this; }
	nullable_ptr_impl( const nullable_ptr_impl<void>& other ) = default;
	nullable_ptr_impl<void>& operator = ( nullable_ptr_impl<void>& other ) = default;

	nullable_ptr_impl( nullable_ptr_impl<void>&& other ) = default;
	nullable_ptr_impl<void>& operator = ( nullable_ptr_impl<void>&& other ) = default;

	nullable_ptr_impl( std::nullptr_t nulp ) : nullable_ptr_base_impl(nulp) {}
	nullable_ptr_impl& operator = ( std::nullptr_t nulp ) { nullable_ptr_base_impl<void>::operator = (nulp); return *this; }

	void swap( nullable_ptr_impl<void>& other )
	{
		nullable_ptr_base_impl::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_impl<void>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == ( const nullable_ptr_impl<T1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_impl<void>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != ( const nullable_ptr_impl<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_impl()
	{
		this->t = nullptr;
	}
};


template<class T, class T1>
T* nullable_cast_impl( nullable_ptr_impl<T1> p ) {
	T* ret = p.get_();
	return ret;
}

template<class T>
T* nullable_cast_impl( nullable_ptr_impl<T> p ) {
	T* ret = p.get_();
	return ret;
}


template<class T, class T1>
nullable_ptr_impl<T1> nullable_cast_impl( T* p ) {
	return nullable_ptr_impl<T1>( p );
}

template<class T>
nullable_ptr_impl<T> nullable_cast_impl( T* p ) {
	return nullable_ptr_impl<T>( p );
}

} // namespace safememory::detail

#endif // SAFE_PTR_IMPL_H
