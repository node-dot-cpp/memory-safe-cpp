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

#include <memory>
#include <stdint.h>
#include <assert.h>

#include <foundation.h>

#define NODECPP_USE_IIBMALLOC
//#define NODECPP_USE_NEW_DELETE_ALLOC

#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#define INLINE_VAR _INLINE_VAR
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#define INLINE_VAR inline
#else
#define NODISCARD
#define INLINE_VAR
#endif

#ifdef NODECPP_USE_IIBMALLOC

#include <iibmalloc.h>
using namespace nodecpp::iibmalloc;
namespace nodecpp::safememory
{
NODECPP_FORCEINLINE void* allocate( size_t sz ) { return g_AllocManager.allocate( sz ); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { g_AllocManager.deallocate( ptr ); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { return g_AllocManager.zombieableAllocate( sz ); }
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { g_AllocManager.zombieableDeallocate( ptr ); }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return g_AllocManager.isZombieablePointerInBlock( allocatedPtr, ptr ); }
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { static_assert(guaranteed_prefix_size <= 3*sizeof(void*)); return guaranteed_prefix_size; }
inline void killAllZombies() { g_AllocManager.killAllZombies(); }
NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return ALIGNMENT; }
} // namespace nodecpp::safememory

#elif defined NODECPP_USE_NEW_DELETE_ALLOC

namespace nodecpp::safememory
{
// NOTE: while being non-optimal, following calls provide safety guarantees and can be used at least for debug purposes
extern thread_local void** zombieList_; // must be set to zero at the beginning of a thread function
inline void killAllZombies()
{
	while ( zombieList_ != nullptr )
	{
		void** next = reinterpret_cast<void**>( *zombieList_ );
		delete [] zombieList_;
		zombieList_ = next;
	}
}
NODECPP_FORCEINLINE void* allocate( size_t sz ) { void* ret = new uint8_t[ sz ]; return ret; }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { delete [] ptr; }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { uint8_t* ret = new uint8_t[ sizeof(uint64_t) + sz ]; *reinterpret_cast<uint64_t*>(ret) = sz; return ret + sizeof(uint64_t);}
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { void** blockStart = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr) - sizeof(uint64_t)); *blockStart = zombieList_;zombieList_ = blockStart; }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return ptr >= allocatedPtr && reinterpret_cast<uint8_t*>(allocatedPtr) + *(reinterpret_cast<uint64_t*>(allocatedPtr) - 1) > reinterpret_cast<uint8_t*>(ptr); }
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { return sizeof(uint64_t); }
NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return sizeof(void*); }
} //namespace nodecpp::safememory

#else
#error at least some specific allocation functionality must be selected
#endif


//#define NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST

namespace nodecpp::safememory
{
enum class MemorySafety {none, partial, full};

//#define NODECPP_MEMORYSAFETY_NONE
#define NODECPP_MEMORYSAFETY_EARLY_DETECTION

#ifdef NODECPP_MEMORYSAFETY_NONE
#define NODECPP_ISSAFE_MODE MemorySafety::none
#define NODECPP_ISSAFE_DEFAULT false
#elif defined NODECPP_MEMORYSAFETY_PARTIAL
#define NODECPP_ISSAFE_MODE MemorySafety::partial
#define NODECPP_ISSAFE_DEFAULT true
#elif defined NODECPP_MEMORYSAFETY_FULL
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#else
#define NODECPP_ISSAFE_MODE MemorySafety::full
#define NODECPP_ISSAFE_DEFAULT true
#endif

#ifndef NODECPP_MEMORYSAFETY_NONE
#ifdef NODECPP_MEMORYSAFETY_EARLY_DETECTION
//constexpr void* invalid_ptr = (void*)(1);
#endif
#endif

#ifdef NODECPP_GCC
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
}

template<class T>
void checkNotNullLargeSize( T* ptr )
{
	if constexpr ( sizeof(T) <= NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE ) ;
	else {
		if ( ptr == nullptr )
			throw std::bad_alloc();
	}
}

template<class T>
void checkNotNullAllSizes( T* ptr )
{
	if ( ptr == nullptr )
		throw std::bad_alloc();
}

inline
void throwPointerOutOfRange()
{
	// TODO: actual implementation
	throw std::bad_alloc();
}


//static_assert( sizeof(void*) == 8 );



template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration
class soft_this_ptr_no_checks; // forward declaration

struct make_owning_t {};
template<class T>
class owning_ptr_no_checks
{
/*template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int>>
	friend owning_ptr<_Ty> make_owning(_Types&&... _Args);*/
	template<class TT, bool isSafe1>
	friend class owning_ptr_no_checks;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base_no_checks;
	template<class TT, bool isSafe1>
	friend class soft_ptr_no_checks;

	T* t;

public:
	owning_ptr_no_checks( make_owning_t, T* t_ ) // make it private with a friend make_owning()!
	{
		t = t_;
	}
	owning_ptr_no_checks() { t = nullptr; }
	owning_ptr( owning_ptr<T>& other ) = delete;
	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>& other ) = delete;
	owning_ptr_no_checks( owning_ptr_no_checks<T>&& other ) {t = other.t; }
	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t;
		other.t = nullptr;
		return *this;
	}
	template<class T1>
	owning_ptr_no_checks( owning_ptr_no_checks<T1, isSafe>&& other )
	{
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
	}
	template<class T1>
	owning_ptr_no_checks& operator = ( owning_ptr_no_checks<T>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
		return *this;
	}
	~owning_ptr_no_checks()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			t->~T();
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t.getTypedPtr()) )
			t->~T();
		t = nullptr;
	}

	void swap( owning_ptr_no_checks<T>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	naked_ptr<T> get() const
	{
		naked_ptr<T> ret;
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

	bool operator == (const soft_ptr_no_checks<T>& other ) const { return t == other; }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_no_checks<T1, isSafe1>& other ) const { return t == other; }

	bool operator != (const soft_ptr_no_checks<T>& other ) const { return t != other; }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_no_checks<T1, isSafe1>& other ) const { return t != other; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t != nullptr; }

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	NODISCARD owning_ptr<_Ty> make_owning_no_checks(_Types&&... _Args)
	{
	uint8_t* data = reinterpret_cast<uint8_t*>( allocate( sizeof(_Ty) ) );
	owning_ptr<_Ty> op(make_owning_t(), (_Ty*)(data);
	thg_stackPtrForMakeOwningCall = dataForObj;
	_Ty* objPtr = new ( data ) _Ty(::std::forward<_Types>(_Args)...);
	return op;
	}



template<class T>
class soft_ptr_base_no_checks
{
	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_static_cast( soft_ptr_no_checks<TT11> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_static_cast( soft_ptr_no_checks<TT> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_reinterpret_cast( soft_ptr_no_checks<TT11> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_reinterpret_cast( soft_ptr_no_checks<TT> );

	T* t;

	soft_ptr_base_no_checks(FirstControlBlock* cb, T* t_) { t = t_; } // to be used for only types annotaded as [[nodecpp::owning_only]]

public:

	soft_ptr_base_no_checks() {}

	template<class T1>
	soft_ptr_base_no_checks( const owning_ptr_no_checks<T1>& owner ) { t = owner.t; }
	template<class T1>
	soft_ptr_base_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner ) { t = owner.t; return *this; }


	template<class T1>
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T1>& other ) { t = owner.t; }
	template<class T1>
	soft_ptr_base_no_checks<T>& operator = ( const soft_ptr_base_no_checks<T1>& other ) { t = owner.t; return *this; }
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T>& other ) { t = other.t; }
	soft_ptr_base_no_checks<T>& operator = ( soft_ptr_base_no_checks<T>& other ) { t = other.t; return *this; }


	soft_ptr_base_no_checks( soft_ptr_base_no_checks<T>&& other ) { t = other.t; other.t = nullptr; }

	soft_ptr_base_no_checks<T>& operator = ( soft_ptr_base_no_checks<T>&& other ) { t = other.t; other.t = nullptr; return *this; }

	template<class T1>
	soft_ptr_base_no_checks( const owning_ptr_no_checks<T1>& owner, T* t_ ) { t = owner.t; }

	template<class T1>
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T1>& other, T* t_ ) { t = t_; }
	soft_ptr_base_no_checks( const soft_ptr_base_no_checks<T>& other, T* t_ ) {t = t_; }

	void swap( soft_ptr_base_no_checks<T>& other )
	{
		T* tmp = other.t;
		other.t = t;
		t = tmp;
	}

	naked_ptr<T> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, t != nullptr );
		naked_ptr<T> ret;
		ret.t = t;
		return ret;
	}

	explicit operator bool() const noexcept
	{
		return t) != nullptr;
	}

	void reset()
	{
		t = nullptr;
	}

	bool operator == (const owning_ptr_no_checks<T>& other ) const { return t == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return t == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_base_no_checks<T>& other ) const { return t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_base_no_checks<T11>& other ) const { return t == other.t; }

	bool operator != (const owning_ptr_no_checks<T>& other ) const { return t != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return t != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_base_no_checks<T>& other ) const { return t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_base_no_checks<T11>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t != nullptr; }

	~soft_ptr_base_no_checks() {}
};

template<class T>
class soft_ptr_no_checks : public soft_ptr_base_no_checks<T>
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr_no_checks<T>;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_static_cast( soft_ptr_no_checks<TT11> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_static_cast( soft_ptr_no_checks<TT> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_reinterpret_cast( soft_ptr_no_checks<TT11> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_reinterpret_cast( soft_ptr_no_checks<TT> );
	friend struct FirstControlBlock;

private:
	friend class soft_this_ptr_no_checks;
	template<class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_in_constructor(TT* ptr);
	friend soft_ptr_no_checks<T> soft_ptr_no_checks_in_constructor(T* ptr);
	soft_ptr_no_checks(FirstControlBlock* cb, T* t) : soft_ptr_base_no_checks<T>(cb, t) {} // to be used for only types annotaded as [[nodecpp::owning_only]]

public:
	soft_ptr_no_checks() : soft_ptr_base_no_checks<T>()
	{
		this->init( soft_ptr_base_no_checks<T>::PointersT::max_data );
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}


	template<class T1>
	soft_ptr_no_checks( const owning_ptr_no_checks<T1>& owner ) : soft_ptr_base_no_checks<T>(owner) {}
	soft_ptr_no_checks( const owning_ptr_no_checks<T>& owner )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			this->initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), this->getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_no_checks<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}
	template<class T1>
	soft_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T1>& owner )
	{
		soft_ptr_base_no_checks<T>::operator = (owner);
		return *this;
	}
	soft_ptr_no_checks<T>& operator = ( const owning_ptr_no_checks<T>& owner )
	{
		bool iWasOnStack = this->isOnStack();
		reset();
		if ( iWasOnStack )
		{
			this->initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
		}
		else
			if ( owner.t .getPtr())
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), this->getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_no_checks<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
		return *this;
	}


	template<class T1>
	soft_ptr_no_checks( const soft_ptr_no_checks<T1>& other ) : soft_ptr_base_no_checks<T>(other) {}
	template<class T1>
	soft_ptr_no_checks<T>& operator = ( const soft_ptr_no_checks<T1>& other ) : soft_ptr_base_no_checks<T>(other) {}
	soft_ptr_no_checks( const soft_ptr_no_checks<T>& other ) : soft_ptr_base_no_checks<T>(other) {}
	soft_ptr_no_checks<T>& operator = ( soft_ptr_no_checks<T>& other )
	{
		soft_ptr_base_no_checks<T>::operator = (other);
		return *this;
	}


	soft_ptr_no_checks( soft_ptr_no_checks<T>&& other ) : soft_ptr_base_no_checks<T>( std::move(other) ) {}

	soft_ptr_no_checks<T>& operator = ( soft_ptr_no_checks<T>&& other )
	{
		soft_ptr_base_no_checks<T>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr_no_checks( const owning_ptr_no_checks<T1>& owner, T* t_ ) : soft_ptr_base_no_checks<T>(owner, t_) {}
	soft_ptr_no_checks( const owning_ptr_no_checks<T>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( t_, owner.t.getPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				init( t_, owner.t.getPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t.getPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}

	template<class T1>
	soft_ptr_no_checks( const soft_ptr_no_checks<T1>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}
	soft_ptr_no_checks( const soft_ptr_no_checks<T>& other, T* t_ ) : soft_ptr_base_no_checks<T>(other, t_) {}

	void swap( soft_ptr_no_checks<T>& other )
	{
		soft_ptr_base_no_checks<T>::swap(other);
	}

	naked_ptr<T> get() const
	{
		return soft_ptr_base_no_checks<T>::get();
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

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	void reset()
	{
		soft_ptr_base_no_checks<T>::reset();
	}

	bool operator == (const owning_ptr_no_checks<T>& other ) const { return this->t == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return this->t == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_no_checks<T>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_no_checks<T11>& other ) const { return this->t == other.t; }

	bool operator != (const owning_ptr_no_checks<T>& other ) const { return this->t != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return this->t != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_no_checks<T>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_no_checks<T11>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t != nullptr; }
};

template<>
class soft_ptr_no_checks<void> : public soft_ptr_base_no_checks<void>
{
	template<class TT>
	friend class owning_ptr_no_checks_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_static_cast( soft_ptr_no_checks<TT> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_static_cast( soft_ptr_no_checks<TT> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_reinterpret_cast( soft_ptr_no_checks<TT> );
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_reinterpret_cast( soft_ptr_no_checks<TT> );
	friend struct FirstControlBlock;
	template<class TT, class TT>
	friend soft_ptr_no_checks<TT> soft_ptr_no_checks_reinterpret_cast( soft_ptr_no_checks<TT> );

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
		soft_ptr_base_no_checks<void>::operator = ( std::move(other) );
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
	bool operator == (const owning_ptr_no_checks<T1>& other ) const { return this->t == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_no_checks<void>& other ) const { return this->t == other.t; }
	template<class T1>
	bool operator == (const soft_ptr_no_checks<T1>& other ) const { return this->t == other.t; }

	template<class T1> 
	bool operator != (const owning_ptr_no_checks<T1>& other ) const { return this->t != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_no_checks<void>& other ) const { return this->t != other.t; }
	template<class T1>
	bool operator != (const soft_ptr_no_checks<T1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->t != nullptr; }

	void reset()
	{
		soft_ptr_base_no_checks<void>::reset();
	}
};

template<class T, class T1>
soft_ptr<T> soft_ptr_static_cast_no_checks( soft_ptr_no_checks<T1> p ) {
	soft_ptr_no_checks<T> ret(p,static_cast<T*>(p.t));
	return ret;
}

template<class T, class T1>
soft_ptr<T> soft_ptr_static_cast_no_checks( soft_ptr<T1> p ) {
	soft_ptr<T> ret(p,static_cast<T*>(p.t));
	return ret;
}

template<class T, class T1, bool isSafe>
soft_ptr<T, isSafe> soft_ptr_reinterpret_cast( soft_ptr<T1, isSafe> p ) {
	soft_ptr<T, isSafe> ret(p,reinterpret_cast<T*>(p.t));
	return ret;
}

template<class T, class T1>
soft_ptr<T> soft_ptr_reinterpret_cast( soft_ptr<T1> p ) {
	soft_ptr<T> ret(p,reinterpret_cast<T*>(p.t));
	return ret;
}


class soft_this_ptr_no_checks
{
	FirstControlBlock* cbPtr = nullptr;
	uint32_t offset;

public:
	soft_this_ptr_no_checks()
	{
		cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
		uintptr_t delta = reinterpret_cast<uint8_t*>(this) - reinterpret_cast<uint8_t*>(cbPtr);
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, delta <= UINT32_MAX, "delta = 0x{:x}", delta );
		offset = (uint32_t)delta;
	}

	soft_this_ptr_no_checks( soft_this_ptr_no_checks& other ) {}
	soft_this_ptr_no_checks& operator = ( soft_this_ptr_no_checks& other ) { return *this; }

	soft_this_ptr_no_checks( soft_this_ptr_no_checks&& other ) {}
	soft_this_ptr_no_checks& operator = ( soft_this_ptr_no_checks&& other ) { return *this; }

	explicit operator bool() const noexcept
	{
		return cbPtr != nullptr;
	}

	template<class T>
	soft_ptr_no_checks<T> getSoftPtr(T* ptr)
	{
		void* allocatedPtr = getAllocatedBlockFromControlBlock_( getAllocatedBlock_(cbPtr) );
		if ( allocatedPtr == nullptr )
			throwPointerOutOfRange();
		//return soft_ptr<T, true>( allocatedPtr, ptr );
		//FirstControlBlock* cb = cbPtr;
		FirstControlBlock* cb = reinterpret_cast<FirstControlBlock*>( reinterpret_cast<uint8_t*>(this) - offset );
		return soft_ptr_no_checks<T, true>( cb, ptr );
	}

	~soft_this_ptr_no_checks()
	{
	}
};

template<class T>
soft_ptr<T> soft_ptr_in_constructor(T* ptr) {
	FirstControlBlock* cbPtr = nullptr;
	cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
	void* allocatedPtr = getAllocatedBlockFromControlBlock_( getAllocatedBlock_(cbPtr) );
	if ( allocatedPtr == nullptr )
		throwPointerOutOfRange();
	FirstControlBlock* cb = cbPtr;
	return soft_ptr<T, true>( cb, ptr );
}


} // namespace nodecpp::safememory

#endif // _NO_CHECKS
