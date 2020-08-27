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

#include "safe_ptr_common.h"
#include <utility>

namespace safe_memory
{

enum class MemorySafety {none, partial, full};

#define NODECPP_ISSAFE_DEFAULT true

/*#ifdef NODECPP_GCC
extern void forcePreviousChangesToThisInDtor( void* p );
#else
#define forcePreviousChangesToThisInDtor(x)
#endif

template<class T>
void destruct( T* t )
{
	if constexpr ( std::is_polymorphic<T>::value )
	{
		auto vpt = nodecpp::platform::backup_vmt_pointer(t);
		t->~T();
		nodecpp::platform::restore_vmt_pointer( t, vpt);
	}
	else
		t->~T();
}*/

template<class T>
void checkNotNullLargeSize( T* ptr )
{
}

template<>
void checkNotNullLargeSize<void>( void* ptr )
{
	// do nothing
}

template<class T>
void checkNotNullAllSizes( T* ptr )
{
}

inline
void throwPointerOutOfRange()
{
	// TODO: actual implementation
}


//static_assert( sizeof(void*) == 8 );

template<class T, bool isSafe> class soft_ptr_base_impl; // forward declaration
template<class T, bool isSafe> class soft_ptr_impl; // forward declaration
template<class T, bool isSafe> class nullable_ptr_base_impl; // forward declaration
template<class T, bool isSafe> class nullable_ptr_impl; // forward declaration
template<class T> class soft_this_ptr_impl; // forward declaration


struct FirstControlBlock {};
FirstControlBlock* getControlBlock_(void*);

//struct make_owning_t {};
template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class owning_ptr_impl
{
/*template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int>>
	friend owning_ptr_impl<_Ty> make_owning_impl(_Types&&... _Args);*/
	template<class TT, bool isSafe1>
	friend class owning_ptr_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_impl;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	template<class TT>
	struct ObjectPointer  {
		TT* t = nullptr; // the only data member!
	public:
		TT* getTypedPtr() const { return t; }
	};
	ObjectPointer<T> t; // the only data member!

public:

	static constexpr bool is_safe = true;

	owning_ptr_impl( make_owning_t, T* t_ ) // make it private with a friend make_owning_impl()!
	{
	}
	owning_ptr_impl()
	{
	}
	owning_ptr_impl( owning_ptr_impl<T, isSafe>& other ) = delete;
	owning_ptr_impl& operator = ( owning_ptr_impl<T, isSafe>& other ) = delete;
	owning_ptr_impl( owning_ptr_impl<T, isSafe>&& other )
	{
	}
	owning_ptr_impl& operator = ( owning_ptr_impl<T, isSafe>&& other )
	{
		return *this;
	}
	template<class T1>
	owning_ptr_impl( owning_ptr_impl<T1, isSafe>&& other )
	{
	}
	template<class T1>
	owning_ptr_impl& operator = ( owning_ptr_impl<T, isSafe>&& other )
	{
		return *this;
	}
	~owning_ptr_impl()
	{
	}

	void reset()
	{
	}

	void swap( owning_ptr_impl<T, isSafe>& other )
	{
	}

	nullable_ptr_impl<T, isSafe> get() const
	{
		nullable_ptr_impl<T, isSafe> ret;
		ret.t = t.getTypedPtr();
		return ret;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( t.getTypedPtr() );
		return *(t.getTypedPtr());
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( t.getTypedPtr() );
		return t.getTypedPtr();
	}

	bool operator == (const soft_ptr_impl<T, isSafe>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_impl<T1, isSafe1>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_impl<T, isSafe>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_impl<T1, isSafe1>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() != nullptr; }

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t.getTypedPtr() != nullptr;
	}
};


extern thread_local void* thg_stackPtrForMakeOwningCall;

template<class _Ty,
	class... _Types>
	NODISCARD owning_ptr_impl<_Ty> make_owning_impl(_Types&&... _Args)
	{
	owning_ptr_impl<_Ty> op(make_owning_t(), (_Ty*)(nullptr));
	return op;
	}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr_base_impl
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_impl;

	/*friend class nullable_ptr_base_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_base_impl;
	friend class nullable_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_impl;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );

	T* pointers = nullptr; // the only data member!
	T* getDereferencablePtr() const { return pointers; }
public:

	static constexpr bool is_safe = true;

	void dbgCheckMySlotConsistency() const {}

	soft_ptr_base_impl() {}

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_impl<T1, isSafe>& owner )
	{
	}
	template<class T1>
	soft_ptr_base_impl<T>& operator = ( const owning_ptr_impl<T1, isSafe>& owner )
	{
		return *this;
	}


	template<class T1>
	soft_ptr_base_impl( const soft_ptr_base_impl<T1, isSafe>& other )
	{
	}
	template<class T1>
	soft_ptr_base_impl<T>& operator = ( const soft_ptr_base_impl<T1, isSafe>& other )
	{
		return *this;
	}
	soft_ptr_base_impl( const soft_ptr_base_impl<T, isSafe>& other )
	{
	}
	soft_ptr_base_impl<T>& operator = ( soft_ptr_base_impl<T, isSafe>& other )
	{
		return *this;
	}


	soft_ptr_base_impl( soft_ptr_base_impl<T, isSafe>&& other )
	{
	}

	soft_ptr_base_impl<T>& operator = ( soft_ptr_base_impl<T, isSafe>&& other )
	{
		return *this;
	}

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_impl<T1, isSafe>& owner, T* t_ )
	{
	}

	template<class T1>
	soft_ptr_base_impl( const soft_ptr_base_impl<T1, isSafe>& other, T* t_ )
	{
	}
	soft_ptr_base_impl( const soft_ptr_base_impl<T, isSafe>& other, T* t_ )
	{
	}

	void swap( soft_ptr_base_impl<T, isSafe>& other )
	{
	}

	nullable_ptr_impl<T, isSafe> get() const
	{
		nullable_ptr_impl<T, isSafe> ret;
		ret.t = getDereferencablePtr();
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return getDereferencablePtr() != nullptr;
	}

	void reset()
	{
	}

	bool operator == (const owning_ptr_impl<T, isSafe>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr_impl<T1, isSafe>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_base_impl<T, isSafe>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_base_impl<T1, isSafe1>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr_impl<T, isSafe>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr_impl<T1, isSafe>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_base_impl<T, isSafe>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_base_impl<T1, isSafe1>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() != nullptr; }

	~soft_ptr_base_impl()
	{
	}
};

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr_impl : public soft_ptr_base_impl<T, isSafe>
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;

	/*friend class nullable_ptr_base_impl<T, isSafe>;
	friend class nullable_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_impl;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_base_impl;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );
	friend struct FirstControlBlock;

private:
	friend class soft_this_ptr_impl<T>;
	template<class TT>
	friend class soft_this_ptr_impl;
	template<class TT>
	friend soft_ptr_impl<TT> soft_ptr_in_constructor_impl(TT* ptr);
	friend soft_ptr_impl<T> soft_ptr_in_constructor_impl(T* ptr);
	soft_ptr_impl(FirstControlBlock* cb, T* t) : soft_ptr_base_impl<T, isSafe>(cb, t) {} // to be used for only types annotaded as [[safe_memory::owning_only]]

public:
	soft_ptr_impl() : soft_ptr_base_impl<T, isSafe>()
	{
	}


	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1, isSafe>& owner ) : soft_ptr_base_impl<T, isSafe>(owner) {}
	soft_ptr_impl( const owning_ptr_impl<T, isSafe>& owner )
	{
	}
	template<class T1>
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T1, isSafe>& owner )
	{
		soft_ptr_base_impl<T, isSafe>::operator = (owner);
		return *this;
	}
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T, isSafe>& owner )
	{
		return *this;
	}


	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1, isSafe>& other ) : soft_ptr_base_impl<T, isSafe>(other) {}
	// template<class T1>
	// soft_ptr_impl<T>& operator = ( const soft_ptr_impl<T1, isSafe>& other ) : soft_ptr_base_impl<T, isSafe>(other) {}
	soft_ptr_impl( const soft_ptr_impl<T, isSafe>& other ) : soft_ptr_base_impl<T, isSafe>(other) {}
	soft_ptr_impl<T>& operator = ( soft_ptr_impl<T, isSafe>& other )
	{
		soft_ptr_base_impl<T, isSafe>::operator = (other);
		return *this;
	}


	soft_ptr_impl( soft_ptr_impl<T, isSafe>&& other ) : soft_ptr_base_impl<T, isSafe>( std::move(other) ) {}

	soft_ptr_impl<T>& operator = ( soft_ptr_impl<T, isSafe>&& other )
	{
		soft_ptr_base_impl<T, isSafe>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1, isSafe>& owner, T* t_ ) : soft_ptr_base_impl<T, isSafe>(owner, t_) {}
	soft_ptr_impl( const owning_ptr_impl<T, isSafe>& owner, T* t_ )
	{
	}

	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1, isSafe>& other, T* t_ ) : soft_ptr_base_impl<T, isSafe>(other, t_) {}
	soft_ptr_impl( const soft_ptr_impl<T, isSafe>& other, T* t_ ) : soft_ptr_base_impl<T, isSafe>(other, t_) {}

	void swap( soft_ptr_impl<T, isSafe>& other )
	{
		soft_ptr_base_impl<T, isSafe>::swap(other);
	}

	nullable_ptr_impl<T, isSafe> get() const
	{
		return soft_ptr_base_impl<T, isSafe>::get();
	}

	T& operator * () const
	{
		checkNotNullAllSizes( this->getDereferencablePtr() );
		return *(this->getDereferencablePtr());
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( this->getDereferencablePtr() );
		return this->getDereferencablePtr();
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->getDereferencablePtr() != nullptr;
	}

	void reset()
	{
		soft_ptr_base_impl<T, isSafe>::reset();
	}

	bool operator == (const owning_ptr_impl<T, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr_impl<T1, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<T, isSafe>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_impl<T1, isSafe1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr_impl<T, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr_impl<T1, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<T, isSafe>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_impl<T1, isSafe1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }
};

template<>
class soft_ptr_impl<void, true> : public soft_ptr_base_impl<void, true>
{
	static constexpr bool isSafe = true;
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	template<class TT, bool isSafe1>
	friend class owning_ptr_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	template<class TT, bool isSafe1>
	friend class soft_ptr_impl;

	/*friend class nullable_ptr_base_impl<void, true>;
	friend class nullable_ptr_impl<void, true>;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_impl;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_base_impl;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, NODECPP_ISSAFE_DEFAULT> );
	friend struct FirstControlBlock;
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr_impl<TT, isSafe1> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1, isSafe1> );
	friend struct FirstControlBlock;

public:
	soft_ptr_impl() : soft_ptr_base_impl<void, isSafe>()
	{
	}


	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1, isSafe>& owner ) : soft_ptr_base_impl<void, isSafe>(owner) {}
	template<class T1>
	soft_ptr_impl<void>& operator = ( const owning_ptr_impl<T1, isSafe>& owner )
	{
		soft_ptr_base_impl<void, isSafe>::operator = (owner);
		return *this;
	}


	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1, isSafe>& other ) : soft_ptr_base_impl<void, isSafe>( other ) {}
	template<class T1>
	soft_ptr_impl<void>& operator = ( const soft_ptr_impl<T1, isSafe>& other )
	{
		soft_ptr_base_impl<void, isSafe>::operator = (other);
		return *this;
	}
	soft_ptr_impl( const soft_ptr_impl<void, isSafe>& other ) : soft_ptr_base_impl<void, isSafe>( other ) {}
	soft_ptr_impl<void>& operator = ( soft_ptr_impl<void, isSafe>& other )
	{
		soft_ptr_base_impl<void, isSafe>::operator = (other);
		return *this;
	}


	soft_ptr_impl( soft_ptr_impl<void, isSafe>&& other ) : soft_ptr_base_impl<void, isSafe>(  std::move(other)  ) {}

	soft_ptr_impl<void>& operator = ( soft_ptr_impl<void, isSafe>&& other )
	{
		soft_ptr_base_impl<void, isSafe>::operator = ( std::move(other) );
		return *this;
	}

	void swap( soft_ptr_impl<void, isSafe>& other )
	{
		soft_ptr_base_impl<void, isSafe>::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->getDereferencablePtr() != nullptr;
	}

	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr_impl<T1, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<void, isSafe>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_impl<T1, isSafe1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr_impl<T1, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<void, isSafe>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_impl<T1, isSafe1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }

	void reset()
	{
		soft_ptr_base_impl<void, isSafe>::reset();
	}
};


template<class T, class T1, bool isSafe>
soft_ptr_impl<T, isSafe> soft_ptr_static_cast_impl( soft_ptr_impl<T1, isSafe> p ) {
	soft_ptr_impl<T, isSafe> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1>
soft_ptr_impl<T, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast_impl( soft_ptr_impl<T1, NODECPP_ISSAFE_DEFAULT> p ) {
	soft_ptr_impl<T, NODECPP_ISSAFE_DEFAULT> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1, bool isSafe>
soft_ptr_impl<T, isSafe> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<T1, isSafe> p ) {
	soft_ptr_impl<T, isSafe> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1>
soft_ptr_impl<T, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<T1, NODECPP_ISSAFE_DEFAULT> p ) {
	soft_ptr_impl<T, NODECPP_ISSAFE_DEFAULT> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}


template<class T>
class soft_this_ptr_impl
{
	FirstControlBlock* cbPtr = nullptr;
	uint32_t offset;

public:

	static constexpr bool is_safe = true;

	soft_this_ptr_impl()
	{
		cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
		uintptr_t delta = reinterpret_cast<uint8_t*>(this) - reinterpret_cast<uint8_t*>(cbPtr);
		NODECPP_ASSERT(safe_memory::module_id, nodecpp::assert::AssertLevel::critical, delta <= UINT32_MAX, "delta = 0x{:x}", delta );
		offset = (uint32_t)delta;
	}

	soft_this_ptr_impl( soft_this_ptr_impl& other ) {}
	soft_this_ptr_impl& operator = ( soft_this_ptr_impl& other ) { return *this; }

	soft_this_ptr_impl( soft_this_ptr_impl&& other ) {}
	soft_this_ptr_impl& operator = ( soft_this_ptr_impl&& other ) { return *this; }

	explicit operator bool() const noexcept
	{
		return cbPtr != nullptr;
	}

	template<class TT>
	soft_ptr_impl<TT> getSoftPtr(TT* ptr)
	{
		void* allocatedPtr = getAllocatedBlockFromControlBlock_( getAllocatedBlock_(cbPtr) );
		if ( allocatedPtr == nullptr )
			throwPointerOutOfRange();
		//return soft_ptr_impl<T, true>( allocatedPtr, ptr );
		//FirstControlBlock* cb = cbPtr;
		FirstControlBlock* cb = reinterpret_cast<FirstControlBlock*>( reinterpret_cast<uint8_t*>(this) - offset );
		return soft_ptr_impl<TT, true>( cb, ptr );
	}

	~soft_this_ptr_impl()
	{
	}
};

template<class T>
soft_ptr_impl<T> soft_ptr_in_constructor_impl(T* ptr) {
	FirstControlBlock* cb = nullptr;
	return soft_ptr_impl<T, true>( cb, ptr );
}

template<class T, bool isSafe = true/*NODECPP_ISSAFE_DEFAULT*/>
class nullable_ptr_base_impl
{
	friend class owning_ptr_impl<T, isSafe>;
	friend class soft_ptr_base_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	//friend class soft_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr_impl;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr_impl;

	template<class TT, bool isSafe1>
	friend class nullable_ptr_base_impl;
	friend class nullable_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class nullable_ptr_impl;

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

	T* t;

public:

	static constexpr bool is_safe = true;

	nullable_ptr_base_impl() { t = nullptr; }

	template<class T1>
	nullable_ptr_base_impl( const owning_ptr_impl<T1, isSafe>& owner ) { *this = owner.get(); }

	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const owning_ptr_impl<T1, isSafe>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_base_impl( const soft_ptr_impl<T1, isSafe>& other ) { *this = other.get(); }
	nullable_ptr_base_impl( const soft_ptr_impl<T, isSafe>& other ) { *this = other.get(); }
	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const soft_ptr_impl<T1, isSafe>& other ) { *this = other.get(); return *this; }
	nullable_ptr_base_impl<T>& operator = ( const soft_ptr_impl<T, isSafe>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_base_impl( const nullable_ptr_base_impl<T1, isSafe>& other ) { t = other.t; }
	template<class T1>
	nullable_ptr_base_impl<T>& operator = ( const nullable_ptr_base_impl<T1, isSafe>& other ) { t = other.t; return *this; }
	nullable_ptr_base_impl( const nullable_ptr_base_impl<T, isSafe>& other ) = default;
	nullable_ptr_base_impl<T, isSafe>& operator = ( const nullable_ptr_base_impl<T, isSafe>& other ) = default;

	nullable_ptr_base_impl( nullable_ptr_base_impl<T, isSafe>&& other ) = default;
	nullable_ptr_base_impl<T, isSafe>& operator = ( nullable_ptr_base_impl<T, isSafe>&& other ) = default;

	void swap( nullable_ptr_base_impl<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get_dereferencable() const 
	{
		checkNotNullLargeSize( t );
		return t;
	}

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}


	bool operator == ( const nullable_ptr_base_impl<T, isSafe>& other ) const { return t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const nullable_ptr_base_impl<T1, isSafe1>& other ) const { return t == other.t; }

	bool operator != ( const nullable_ptr_base_impl<T, isSafe>& other ) const { return t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const nullable_ptr_base_impl<T1, isSafe1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return t != nullptr; }

	~nullable_ptr_base_impl()
	{
		t = nullptr;
	}
};


template<class T, bool isSafe = true/*NODECPP_ISSAFE_DEFAULT*/>
class nullable_ptr_impl : public nullable_ptr_base_impl<T, isSafe>
{
	friend class owning_ptr_impl<T, isSafe>;
	friend class soft_ptr_base_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	//friend class soft_ptr_impl<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr_impl;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr_impl;

	/*friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class owning_ptr_no_checks;
	friend class soft_ptr_base_impl_no_checks<T>;
	template<class TT>
	friend class soft_ptr_base_impl_no_checks;*/

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

public:
	nullable_ptr_impl() : nullable_ptr_base_impl<T, isSafe>() {}

	nullable_ptr_impl(T& t_) { this->t = &t_; }

	template<class T1>
	nullable_ptr_impl( const owning_ptr_impl<T1, isSafe>& owner ) : nullable_ptr_base_impl<T, isSafe>(owner) {}
	nullable_ptr_impl( const owning_ptr_impl<T, isSafe>& owner ) : nullable_ptr_base_impl<T, isSafe>() {*this = owner.get();}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const owning_ptr_impl<T1, isSafe>& owner ) { *this = owner.get(); return *this; }
	nullable_ptr_impl<T>& operator = ( const owning_ptr_impl<T, isSafe>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const soft_ptr_impl<T1, isSafe>& other ) : nullable_ptr_base_impl<T, isSafe>(other) {}
	nullable_ptr_impl( const soft_ptr_impl<T, isSafe>& other ) : nullable_ptr_base_impl<T, isSafe>(other) {}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const soft_ptr_impl<T1, isSafe>& other ) { *this = other.get(); return *this; }
	nullable_ptr_impl<T>& operator = ( const soft_ptr_impl<T, isSafe>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const nullable_ptr_impl<T1, isSafe>& other ) : nullable_ptr_base_impl<T, isSafe>(other) {}
	template<class T1>
	nullable_ptr_impl<T>& operator = ( const nullable_ptr_impl<T1, isSafe>& other ) { this->t = other.t; return *this; }
	nullable_ptr_impl( const nullable_ptr_impl<T, isSafe>& other ) = default;
	nullable_ptr_impl<T, isSafe>& operator = ( const nullable_ptr_impl<T, isSafe>& other ) = default;

	nullable_ptr_impl( nullable_ptr_impl<T, isSafe>&& other ) = default;
	nullable_ptr_impl<T, isSafe>& operator = ( nullable_ptr_impl<T, isSafe>&& other ) = default;

	void swap( nullable_ptr_impl<T, isSafe>& other )
	{
		nullable_ptr_base_impl<T, isSafe>::swap( other );
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

	T* get_dereferencable() const 
	{
		checkNotNullLargeSize( this->t );
		return this->t;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_impl<T, isSafe>& other ) const { return this->t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const nullable_ptr_impl<T1, isSafe1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_impl<T, isSafe>& other ) const { return this->t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const nullable_ptr_impl<T1, isSafe1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_impl()
	{
		this->t = nullptr;
	}
};


template<>
class nullable_ptr_impl<void, true> : public nullable_ptr_base_impl<void, true>
{
	template<class TT, bool isSafe1>
	friend class owning_ptr_impl;
	friend class soft_ptr_base_impl<void, true>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_impl;
	//friend class soft_ptr_impl<void, true>;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr_impl;

public:
	nullable_ptr_impl() : nullable_ptr_base_impl() {}

	template<class T1>
	nullable_ptr_impl(T1& t_) : nullable_ptr_base_impl<void>(t_) {}

	template<class T1>
	nullable_ptr_impl( const owning_ptr_impl<T1, true>& owner ) : nullable_ptr_base_impl(owner) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const owning_ptr_impl<T1, true>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const soft_ptr_impl<T1, true>& other ) : nullable_ptr_base_impl(other) {}
	nullable_ptr_impl( const soft_ptr_impl<void, true>& other ) : nullable_ptr_base_impl(other) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const soft_ptr_impl<T1, true>& other ) { *this = other.get(); return *this; }
	nullable_ptr_impl<void>& operator = ( const soft_ptr_impl<void, true>& other ) { *this = other.get(); return *this; }

	template<class T1>
	nullable_ptr_impl( const nullable_ptr_impl<T1, true>& other ) : nullable_ptr_base_impl(other) {}
	template<class T1>
	nullable_ptr_impl<void>& operator = ( const nullable_ptr_impl<T1, true>& other ) { t = other.t; return *this; }
	nullable_ptr_impl( const nullable_ptr_impl<void, true>& other ) = default;
	nullable_ptr_impl<void, true>& operator = ( nullable_ptr_impl<void, true>& other ) = default;

	nullable_ptr_impl( nullable_ptr_impl<void, true>&& other ) = default;
	nullable_ptr_impl<void, true>& operator = ( nullable_ptr_impl<void, true>&& other ) = default;

	void swap( nullable_ptr_impl<void, true>& other )
	{
		nullable_ptr_base_impl::swap( other );
	}

	void* get_dereferencable() const 
	{
		checkNotNullLargeSize( this->t );
		return this->t;
	}

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	bool operator == ( const nullable_ptr_impl<void, true>& other ) const { return this->t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const nullable_ptr_impl<T1, isSafe1>& other ) const { return this->t == other.t; }

	bool operator != ( const nullable_ptr_impl<void, true>& other ) const { return this->t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const nullable_ptr_impl<T1, isSafe1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~nullable_ptr_impl()
	{
		this->t = nullptr;
	}
};

} // namespace safe_memory

#endif // SAFE_PTR_IMPL_H
