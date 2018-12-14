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

#ifndef SAFE_PTR_H
#define SAFE_PTR_H

#include <memory>
#include <stdint.h>
#include <assert.h>

#include "foundation/include/foundation.h"

#define USE_IIBMALLOC
#if defined _MSC_VER
#define NODISCARD _NODISCARD
#define INLINE_VAR _INLINE_VAR
#elif defined __GNUC__
#define NODISCARD [[nodiscard]]
#define INLINE_VAR inline
#else
#define NODISCARD
#define INLINE_VAR
#endif

#ifdef USE_IIBMALLOC
#include "iibmalloc/src/iibmalloc.h"
NODECPP_FORCEINLINE void* allocate( size_t sz ) { return g_AllocManager.allocate( sz ); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { g_AllocManager.deallocate( ptr ); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { return g_AllocManager.zombieableAllocate( sz ); }
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { g_AllocManager.zombieableDeallocate( ptr ); }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return g_AllocManager.isZombieablePointerInBlock( allocatedPtr, ptr ); }
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { static_assert(guaranteed_prefix_size <= 3*sizeof(void*)); return guaranteed_prefix_size; }
#else
// NOTE: following calls make not too much practical sense and can be somehow used for debug purposes only
NODECPP_FORCEINLINE void* allocate( size_t sz ) { void* ret = new uint8_t[ sz ]; memset( ret, 0, sz ); return ret; }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { delete [] ptr; }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { uint8_t* ret = new uint8_t[ sizeof(uint64_t) + sz ]; memset( ret, 0, sizeof(uint64_t) + sz ); *reinterpret_cast<uint64_t*>(ret) = sz; return ret + sizeof(uint64_t);}
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { delete [] (reinterpret_cast<uint8_t*>(ptr) - sizeof(uint64_t)); }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return ptr >= allocatedPtr && reinterpret_cast<uint8_t*>(allocatedPtr) + *(reinterpret_cast<uint64_t*>(allocatedPtr) - 1) > reinterpret_cast<uint8_t*>(ptr); }
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { return 0; }
#endif


#if defined __GNUC__
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

//#define NODECPP_HUGE_SIZE_OF_SAFE_PTR_LIST

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

#define CONTROL_BLOCK_SIZE 4096 // TODO: make platform-dependent consideration

extern void forcePreviousChangesToThisInDtor( void* p ); // TODO: if not gcc, just #define forcePreviousChangesToThisInDtor(x)


/*NODECPP_FORCEINLINE
void* readVMT(void* p) { return *((void**)p); }

NODECPP_FORCEINLINE
void restoreVMT(void* p, void* vpt) { *((void**)p) = vpt; }

NODECPP_FORCEINLINE
std::pair<size_t, size_t> getVMPPos(void* p) { return std::make_pair( 0, sizeof(void*) ); }*/

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

/*NODECPP_FORCEINLINE
bool isGuaranteedOnStack( void* ptr )
{
	int a;
	constexpr uintptr_t upperBitsMask = ~( CONTROL_BLOCK_SIZE - 1 );
	return ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0;
}*/

template<class T>
void checkNotNullLargeSize( T* ptr )
{
	if constexpr ( sizeof(T) <= CONTROL_BLOCK_SIZE ) ;
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

struct FirstControlBlock // not reallocatable
{
	struct PtrWishFlagsForSoftPtrList : public nodecpp::platform::allocated_ptr_with_flags<2> {
	public:
		void setPtr( void* ptr_ ) { nodecpp::platform::allocated_ptr_with_flags<2>::init(ptr_); }
		void* getPtr() { return nodecpp::platform::allocated_ptr_with_flags<2>::get_ptr(); }
		void setUsed() { set_flag<0>(); }
		void setUnused() { unset_flag<0>(); }
		bool isUsed() { return has_flag<0>(); }
		void set1stBlock() { set_flag<1>(); }
		void set2ndBlock() { unset_flag<1>(); }
		bool is1stBlock() { return has_flag<1>(); }
		static bool is1stBlock( uintptr_t ptr ) { return (ptr & 2)>>1; }
	};
	static_assert( sizeof(PtrWishFlagsForSoftPtrList) == 8 );

	struct SecondCBHeader
	{
		static constexpr size_t secondBlockStartSize = 8;	
		PtrWishFlagsForSoftPtrList* firstFree;
		size_t otherAllockedCnt;
		PtrWishFlagsForSoftPtrList slots[1];
		void addToFreeList( PtrWishFlagsForSoftPtrList* begin, size_t count ) {
			//assert( firstFree == nullptr );
			firstFree = begin;
			for ( size_t i=0; i<count-1; ++i ) {
				begin[i].setPtr(begin + i + 1);
				begin[i].set2ndBlock();
			}
			begin[count-1].setPtr(nullptr);
			begin[count-1].set2ndBlock();
			//dbgCheckFreeList();
		}
		size_t insert( void* ptr ) {
			assert( firstFree != nullptr );
			PtrWishFlagsForSoftPtrList* tmp = ((PtrWishFlagsForSoftPtrList*)(firstFree->getPtr()));
			assert( !firstFree->isUsed() );
			size_t idx;
			assert( !firstFree->is1stBlock() );
			idx = firstFree - slots;
			firstFree->setPtr(ptr);
			firstFree->setUsed();
			firstFree = tmp;
			assert( firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
			assert( idx < (1<<19) ); // TODO
				printf( "at 2nd block 0x%zx: inserted idx %zd with 0x%zx\n", (size_t)this, idx, (size_t)ptr);
			return idx;
		}
		void resetPtr( size_t idx, void* newPtr ) {
				printf( "at 2nd block 0x%zx: about to reset idx %zd to 0x%zx\n", (size_t)this, idx, (size_t)newPtr);
			assert( idx < otherAllockedCnt );
			slots[idx].setPtr( newPtr );
			slots[idx].setUsed();
			slots[idx].set2ndBlock();
			//dbgCheckFreeList();
			//assert( firstFree == nullptr || !firstFree->isUsed() );
		}
		void remove( size_t idx ) {
			assert( firstFree == nullptr || !firstFree->isUsed() );
			assert( idx < otherAllockedCnt );
			slots[idx].setPtr( firstFree );
			firstFree = slots + idx;
			firstFree->setUnused();
			firstFree->set2ndBlock();
			assert( firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
		}
		static SecondCBHeader* reallocate(SecondCBHeader* present )
		{
			if ( present != nullptr ) {
				assert( present->otherAllockedCnt != 0 );
				size_t newSize = (present->otherAllockedCnt << 1) + 2;
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (newSize + 2) * sizeof(PtrWishFlagsForSoftPtrList) ) );
				//PtrWishFlagsForSoftPtrList* newOtherAllockedSlots = 
				memcpy( ret->slots, present->slots, sizeof(PtrWishFlagsForSoftPtrList) * present->otherAllockedCnt );
				deallocate( present );
				//otherAllockedSlots.setPtr( newOtherAllockedSlots );
				ret->addToFreeList( ret->slots + present->otherAllockedCnt, newSize - present->otherAllockedCnt );
				ret->otherAllockedCnt = newSize;
				printf( "after 2nd block relocation: ret = 0x%zx, ret->otherAllockedCnt = %zd (reallocation)\n", (size_t)ret, ret->otherAllockedCnt );
				return ret;
			}
			else {
				//assert( otherAllockedCnt == 0 );
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (secondBlockStartSize + 2) * sizeof(PtrWishFlagsForSoftPtrList) ) );
				ret->otherAllockedCnt = secondBlockStartSize;
				//present->firstFree->set(nullptr);
				//otherAllockedSlots.setPtr( reinterpret_cast<PtrWishFlagsForSoftPtrList*>( allocate( otherAllockedCnt * sizeof(PtrWishFlagsForSoftPtrList) ) ) );
				ret->addToFreeList( ret->slots, secondBlockStartSize );
				printf( "after 2nd block relocation: ret = 0x%zx, ret->otherAllockedCnt = %zd (ini allocation)\n", (size_t)ret, ret->otherAllockedCnt );
				return ret;
			}
		}
		void dealloc()
		{
			deallocate( this );
		}
	};

	struct PtrWithMaskAndFlag : protected nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>
	{
		void init() { nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::init(); }
		void setPtr(SecondCBHeader* ptr) { nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::set_ptr( ptr ); }
		SecondCBHeader* getPtr() { return reinterpret_cast<SecondCBHeader*>( nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::get_ptr() ); }
		uint32_t getMask() { return (uint32_t)(nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::get_mask()); }
		void setMask(size_t mask) { return nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::set_mask(mask); }
		void setZombie() { nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::init(); nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::set_flag<0>(); }
		bool isZombie() { return nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::has_flag<0>(); }
	};

	static constexpr size_t maxSlots = 3;
	PtrWishFlagsForSoftPtrList slots[maxSlots];
	static constexpr size_t secondBlockStartSize = 8;	
	//PtrWishFlagsForSoftPtrList* firstFree;
	//size_t otherAllockedCnt = 0; // TODO: try to rely on our allocator on deriving this value
//	nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1> otherAllockedSlots;
	PtrWithMaskAndFlag otherAllockedSlots;

	void dbgCheckFreeList() {
		/*PtrWishFlagsForSoftPtrList* start = firstFree;
		while( start ) {
			assert( !start->isUsed() );
			assert( ( start->getPtr() == 0 || start->is1stBlock() && (size_t)(start - slots) < maxSlots ) || ( (!start->is1stBlock()) && (size_t)(start - otherAllockedSlots.getPtr()) < otherAllockedCnt ) );
			start = ((PtrWishFlagsForSoftPtrList*)(start->getPtr()));
		}*/
	}

	void init() {
		//firstFree = slots;
		for ( size_t i=0; i<maxSlots-1; ++i ) {
			slots[i].setPtr(slots + i + 1);
			slots[i].set1stBlock();
		}
		slots[maxSlots-1].setPtr(nullptr);
		slots[maxSlots-1].set1stBlock();

		//otherAllockedCnt = 0;
		otherAllockedSlots.init();
		//assert( !firstFree->isUsed() );
		dbgCheckFreeList();
		printf( "1CB initialized at 0x%zx, otherAllockedSlots.getPtr() = 0x%zx\n", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
	}
	/*void deinit() {
		if ( otherAllockedSlots.getPtr() != nullptr ) {
			assert( otherAllockedCnt != 0 );
			//delete [] otherAllockedSlots;
			deallocate( otherAllockedSlots.getPtr() );
			otherAllockedCnt = 0;
		}
		else {
			assert( otherAllockedCnt == 0 );
		}
	}*/
	void addToFreeList( PtrWishFlagsForSoftPtrList* begin, size_t count ) {
		//assert( firstFree == nullptr );
		//firstFree = begin;
		for ( size_t i=0; i<count-1; ++i ) {
			begin[i].setPtr(begin + i + 1);
			begin[i].set2ndBlock();
		}
		begin[count-1].setPtr(nullptr);
		begin[count-1].set2ndBlock();
		dbgCheckFreeList();
	}
	void enlargeSecondBlock() {
		otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
	}
	size_t insert( void* ptr ) {
		uint32_t mask = otherAllockedSlots.getMask();
		//if ( mask != 0x7 )
		{
			// TODO: optimize!
			for ( size_t i=0; i<3; ++i )
				if ( ( mask & (((size_t)1)<<i)) == 0 )
				{
					mask |= (((size_t)1)<<i);
					slots[i].setPtr(ptr);
					slots[i].setUsed();
					otherAllockedSlots.setMask( mask );
					printf( "1CB 0x%zx: inserted 0x%zx at idx %zd\n", (size_t)this, (size_t)ptr, i );
					return i;
				}
		}
		//else
		{
			if ( otherAllockedSlots.getPtr() == nullptr || otherAllockedSlots.getPtr()->firstFree == nullptr )
			{
		printf( "1CB 0x%zx: about to reset 2nd block, otherAllockedSlots.getPtr() = 0x%zx\n", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
				otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
		printf( "1CB 0x%zx: after reset 2nd block, otherAllockedSlots.getPtr() = 0x%zx\n", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
			}
			assert ( otherAllockedSlots.getPtr() && otherAllockedSlots.getPtr()->firstFree );
			size_t idx = maxSlots + otherAllockedSlots.getPtr()->insert( ptr );
					printf( "1CB 0x%zx: inserted 0x%zx at idx %zd\n", (size_t)this, (size_t)ptr, idx );
			return idx;
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		printf( "1CB 0x%zx: about to reset to 0x%zx at idx %zd\n", (size_t)this, (size_t)newPtr, idx );
		if ( idx < maxSlots ) {
			slots[idx].setPtr( newPtr );
			slots[idx].setUsed();
			slots[idx].set1stBlock();
		}
		else {
			//assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots.getPtr()->resetPtr( idx, newPtr );
		}
		//dbgCheckFreeList();
	}
	void remove( size_t idx ) {
		printf( "1CB 0x%zx: about to remove at idx %zd\n", (size_t)this, idx );
		if ( idx < maxSlots ) {
			assert( slots[idx].isUsed() );
			slots[idx].setUnused();
			slots[idx].set1stBlock();
			otherAllockedSlots.setMask( otherAllockedSlots.getMask() & ~(1<<idx) );
		}
		else {
			//assert( idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			assert( otherAllockedSlots.getPtr() != nullptr );
			otherAllockedSlots.getPtr()->remove( idx );
		}
		//assert( firstFree == nullptr || !firstFree->isUsed() );
		//dbgCheckFreeList();
	}
	void clear() {
		printf( "1CB 0x%zx: clear(), otherAllockedSlots.getPtr() = 0x%zx\n", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
		if ( otherAllockedSlots.getPtr() != nullptr )
			otherAllockedSlots.getPtr()->dealloc();
		otherAllockedSlots.setZombie();
	}
	bool isZombie() { return otherAllockedSlots.isZombie(); }
};
//static_assert( sizeof(FirstControlBlock) == 32 );



inline
FirstControlBlock* getControlBlock_(void* t) { return reinterpret_cast<FirstControlBlock*>(t) - 1; }
inline
uint8_t* getAllocatedBlock_(void* t) { return reinterpret_cast<uint8_t*>(getControlBlock_(t)) + getPrefixByteCount(); }


template<class T, bool isSafe> class soft_ptr; // forward declaration
template<class T, bool isSafe> class naked_ptr; // forward declaration

/*template<bool _Test,
	class _Ty = void>
	using enable_if_t = typename std::enable_if<_Test, _Ty>::type;*/

//template<class _Ty>
//	_INLINE_VAR constexpr bool is_array_v = std::is_array<_Ty>::value;

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class owning_ptr
{
/*template<class _Ty,
	class... _Types,
	enable_if_t<!is_array_v<_Ty>, int>>
	friend owning_ptr<_Ty> make_owning(_Types&&... _Args);*/
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	T* t;
	FirstControlBlock* getControlBlock() { return getControlBlock_(t); }
	uint8_t* getAllocatedBlock() {return getAllocatedBlock_(t); }

	void updatePtrForListItemsWithInvalidPtr()
	{
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
				reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->invalidatePtr();
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
					reinterpret_cast<soft_ptr<T, isSafe>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->invalidatePtr();
	}

public:

	owning_ptr( T* t_ )
	{
		t = t_;
		getControlBlock()->init();
	}
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr( owning_ptr<T, isSafe>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other )
	{
		t = other.t;
		other.t = nullptr;
		return *this;
	}
	~owning_ptr()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t) )
		{
			updatePtrForListItemsWithInvalidPtr();
			destruct( t );
			zombieDeallocate( getAllocatedBlock_(t) );
			getControlBlock()->clear();
			t = nullptr; 
			forcePreviousChangesToThisInDtor(this); // force compilers to apply the above instruction
		}
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t) )
		{
			updatePtrForListItemsWithInvalidPtr();
			destruct( t );
			zombieDeallocate( getAllocatedBlock_(t) );
			getControlBlock()->clear();
			t = nullptr;
		}
	}

	void reset( T* t_ ) // Q: what happens to safe ptrs?
	{
		assert( t == nullptr );
		reset();
	}

	void swap( owning_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	naked_ptr<T, isSafe> get() const
	{
		naked_ptr<T, isSafe> ret;
		ret.t = t;
		return ret;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( t );
		return *t;
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( t );
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};

#if 0
template<class T>
class owning_ptr<T>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class soft_ptr<T>;
	T* t;

public:
	owning_ptr()
	{
		t = nullptr;
	}
	owning_ptr( T* t_ )
	{
		t = t_;
	}
	owning_ptr( owning_ptr<T, false>& other ) = delete;
	owning_ptr( owning_ptr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
	}
	~owning_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: being placed at the level of class definition, the codition may be checked whether or not this specialization is instantiated (see, for instance, https://stackoverflow.com/questions/5246049/c11-static-assert-and-template-instantiation)
		if ( NODECPP_LIKELY(t) )
		{
			delete t;
		}
	}

	owning_ptr& operator = ( owning_ptr<T, false>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, false>&& other )
	{
		t = other.t;
		other.t = nullptr;
		return *this;
	}

	void reset( T* t_ = t )
	{
		T* tmp = t;
		t = t_;
		// if ( NODECPP_LIKELY(tmp) ) : we do not need this check
		delete tmp;
	}

	void swap( owning_ptr<T, false>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T* get() const
	{
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}
};
#endif // 0

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array_v<_Ty>, int> = 0>
	NODISCARD owning_ptr<_Ty> make_owning(_Types&&... _Args)
	{	// make a unique_ptr
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	_Ty* objPtr = new ( data + sizeof(FirstControlBlock) - getPrefixByteCount() ) _Ty(::std::forward<_Types>(_Args)...);
	return owning_ptr<_Ty>(objPtr);
	}



#define SAFE_PTR_DEBUG_MODE

#ifdef SAFE_PTR_DEBUG_MODE
extern thread_local size_t onStackSafePtrCreationCount; 
extern thread_local size_t onStackSafePtrDestructionCount;
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {++onStackSafePtrCreationCount;}
//#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {if ( bornOnStack ) { /*assert( nodecpp::platform::is_guaranteed_on_stack( this ) );*/ ++onStackSafePtrDestructionCount;}}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() { if ( isOnStack() ) {++onStackSafePtrDestructionCount;} }
#else
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {}
#endif // SAFE_PTR_DEBUG_MODE


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr;
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );

	//Ptr2PtrWishData td;
#ifndef SAFE_PTR_DEBUG_MODE
	T* derefPtr;
	T* getPtr_() const { return derefPtr; }
	void setPtr_(T* ptr) { derefPtr = ptr; }
	void resetPtr_(T* ptr) { derefPtr = ptr; }
	void invalidatePtr() { td.init( nullptr, Ptr2PtrWishData::invalidData ); derefPtr = nullptr; }
	void setOnStack() {}
#else
	/*struct PtrWishOnStackFlag : public nodecpp::platform::allocated_ptr_with_flags<1> {
	public:
		void init( void* ptr_ ) { nodecpp::platform::allocated_ptr_with_flags<1>::init(ptr_); }
		void resetPtr( void* ptr_ ) { nodecpp::platform::allocated_ptr_with_flags<1>::set_ptr(ptr_); }
		void* getPtr() const { return nodecpp::platform::allocated_ptr_with_flags<1>::get_ptr(); }
		void setOnStack() { set_flag<0>(); }
		bool isOnStack() { return has_flag<0>(); }
	};
    struct PtrWishOnStackFlag : public Ptr2PtrWishFlags {
        void init( void* ptr_ ) { Ptr2PtrWishFlags::init(ptr_); }
        void resetPtr( void* ptr_ ) { Ptr2PtrWishFlags::resetPtr(ptr_); }
        void* getPtr() const { return Ptr2PtrWishFlags::getPtr(); }
        void setOnStack() { setFlag(0); }
        bool isOnStack() { return isFlag(0); }
	};
	static_assert( sizeof(PtrWishOnStackFlag) == 8 );
	PtrWishOnStackFlag derefPtr;
	T* getPtr_() const { return reinterpret_cast<T*>( derefPtr.getPtr() ); }
	void setPtr_(T* ptr) { derefPtr.init(reinterpret_cast<void*>(ptr)); }
	void resetPtr_(T* ptr) { derefPtr.resetPtr(reinterpret_cast<void*>(ptr)); }
	void invalidatePtr() { td.init( nullptr, Ptr2PtrWishData::invalidData ); derefPtr.resetPtr(nullptr); }
	void setOnStack() { derefPtr.setOnStack(); }*/
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
	PointersT pointers;
	T* getDereferencablePtr() const { return reinterpret_cast<T*>( pointers.get_ptr() ); }
	void* getAllocatedPtr() const {return pointers.get_allocated_ptr(); }
	void init( T* ptr, T* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }
	template<class T1>
	void init( T* ptr, T1* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }

	//void setPtr_(T* ptr) { derefPtr.init(reinterpret_cast<void*>(ptr)); }
	//void resetPtr_(T* ptr) { derefPtr.resetPtr(reinterpret_cast<void*>(ptr)); }
	void invalidatePtr() { pointers.set_ptr(nullptr); pointers.set_allocated_ptr(nullptr); pointers.set_data(PointersT::max_data); }
	void setOnStack() { pointers.set_flag<0>(); }
	bool isOnStack() { return pointers.has_flag<0>(); }
#endif // SAFE_PTR_DEBUG_MODE

	/*size_t getIdx_() const { return td.getData(); }
	FirstControlBlock* getControlBlock() const { return getControlBlock_(getAllocatedPtr); }*/
	size_t getIdx_() const { return pointers.get_data(); }
	FirstControlBlock* getControlBlock() const { return getControlBlock_(getAllocatedPtr()); }
	static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

public:
	soft_ptr()
	{
		pointers.init( PointersT::max_data );
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		printf( "1 created soft_ptr at 0x%zx\n", (size_t)this );
	}


	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t )
				init( owner.t, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "2 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	soft_ptr( const owning_ptr<T, isSafe>& owner )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t )
				init( owner.t, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "3 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	template<class T1>
	soft_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner )
	{
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
		}
		else
			if ( owner.t )
				init( owner.t, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		return *this;
	}
	soft_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner )
	{
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
		}
		else
			if ( owner.t )
				init( owner.t, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		return *this;
	}


	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "4 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	template<class T1>
	soft_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other )
	{
		if ( this == &other ) return;
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		return *this;
	}
	soft_ptr( const soft_ptr<T, isSafe>& other )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "5 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>& other )
	{
		if ( this == &other ) return *this;
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		return *this;
	}


	soft_ptr( soft_ptr<T, isSafe>&& other )
	{
		if ( this == &other ) return;
		bool otherOnStack = other.isOnStack();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			/*if ( other.getIdx_() != PointersT::max_data )
			{
				other.getControlBlock()->remove(other.getIdx_());
				other.pointers.init(PointersT::max_data);
			}
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();*/
			init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
			if ( other.getIdx_() != PointersT::max_data )
				other.getControlBlock()->remove(other.getIdx_());
			other.pointers.init(PointersT::max_data);
			if ( otherOnStack)
				other.setOnStack();
		}
		else
		{
			if ( otherOnStack)
			{
				if ( other.getDereferencablePtr() )
					init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
				else
					init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				other.pointers.init(PointersT::max_data);
				other.setOnStack();
			}
			else
			{
				pointers = other.pointers;
				if ( other.getDereferencablePtr() )
					getControlBlock(getAllocatedPtr())->resetPtr(getIdx_(), this);
				other.pointers.init(PointersT::max_data);
			}
		}
		printf( "6 created soft_ptr at 0x%zx\n", (size_t)this );
	}

	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>&& other )
	{
		// TODO+++: revise
		if ( this == &other ) return;
		bool wasOnStack = isOnStack();
		reset();
		if ( wasOnStack )
		{
			assert( getIdx_() == PointersT::max_data );
			if ( other.isOnStack() )
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				other.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				if ( other.getIdx_() != PointersT::max_data )
					other.getControlBlock()->remove(other.getIdx_());
				other.init( PointersT::max_data );
			}
			setOnStack();
		}
		else
		{
			if ( other.isOnStack() )
			{
				if ( other.getDereferencablePtr() )
					init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
				else
					init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				other.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				pointers = other.pointers;
				if ( getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				other.init( PointersT::max_data );
			}
		}
		return *this;
	}

	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( t_, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t )
				init( t_, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "7 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	soft_ptr( const owning_ptr<T, isSafe>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( t_, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t )
				init( t_, owner.t, getControlBlock(owner.t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t, PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "8 created soft_ptr at 0x%zx\n", (size_t)this );
	}

	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( t_, other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "9 created soft_ptr at 0x%zx\n", (size_t)this );
	}
	soft_ptr( const soft_ptr<T, isSafe>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( t_, other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		printf( "10 created soft_ptr at 0x%zx\n", (size_t)this );
	}

#if 0
	void swap( soft_ptr<T, isSafe>& other )
	{return;
		bool iWasOnStack = isOnStack();
		bool otherWasOnStack = other.isOnStack();
		auto tmp = pointers;
		pointers = other.pointers;
		pointers = tmp;
		if ( iWasOnStack )
		{
			if ( otherWasOnStack )
			{
			}
			else
			{
				////assert( getDereferencablePtr() && getIdx_() != PointersT::max_data );
				getControlBlock()->insert(this);
			}
		}
		else
		{
			if ( otherWasOnStack )
			{
			}
			else
			{
				if ( getDereferencablePtr() && getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				if ( other.getDereferencablePtr() && other.getIdx_() != PointersT::max_data)
					other.getControlBlock()->resetPtr(other.getIdx_(), &other);
			}
		}
		/*auto tmp = getDereferencablePtr();
		resetPtr_(other.getDereferencablePtr());
		other.resetPtr_(tmp);
		td.swap( other.td );*/
		auto tmp = pointers;
		other.pointers = pointers;
		pointers = tmp;
		if ( getDereferencablePtr() && getIdx_() != PointersT::max_data )
			getControlBlock()->resetPtr(getIdx_(), this);
		if ( other.getDereferencablePtr() && other.getIdx_() != PointersT::max_data)
			other.getControlBlock()->resetPtr(other.getIdx_(), &other);
	}
#endif

	naked_ptr<T, isSafe> get() const
	{
		assert( getDereferencablePtr() != nullptr );
		naked_ptr<T, isSafe> ret;
		ret.t = getDereferencablePtr();
		return ret;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( getDereferencablePtr() );
		return *getDereferencablePtr();
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( getDereferencablePtr() );
		return getDereferencablePtr();
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return getDereferencablePtr() != nullptr;
	}

	void reset()
	{
		if( getDereferencablePtr() != nullptr ) {
			//assert( getIdx_() != Ptr2PtrWishData::invalidData );
			if ( getIdx_() != PointersT::max_data )
				getControlBlock()->remove(getIdx_());
			invalidatePtr();
		}
	}

	~soft_ptr()
	{
		printf( "about to delete soft_ptr at 0x%zx\n", (size_t)this );
		INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT()
		if( getDereferencablePtr() != nullptr ) {
			//assert( getIdx_() != Ptr2PtrWishData::invalidData );
			assert( getAllocatedPtr() );
			if(getIdx_()>=3 && getIdx_() != PointersT::max_data)
			{
			printf( "getIdx_() in dtor: 0x%zx\n", getIdx_() ); // otherAllockedSlots.getPtr()
			printf( "getAllocatedPtr() in dtor: 0x%zx\n", (size_t)(getAllocatedPtr()) ); // otherAllockedSlots.getPtr()
			printf( "getControlBlock() in dtor: 0x%zx\n", (size_t)(getControlBlock()) ); // 
			printf( "getControlBlock()->otherAllockedSlots.getPtr() in dtor: 0x%zx\n", (size_t)(getControlBlock()->otherAllockedSlots.getPtr()) ); // 
			if ( getControlBlock()->otherAllockedSlots.getPtr() == 0 )
			printf( "isOnStack() in dtor: %s\n", isOnStack() ? "YES :(" : "NO" ); // 
			}
			if ( getIdx_() != PointersT::max_data )
			{
				assert(!isOnStack());
				getControlBlock()->remove(getIdx_());
			}
			invalidatePtr();
			forcePreviousChangesToThisInDtor(this); // force compilers to apply the above instruction
		}
	}
};

#if 0
template<class T>
class soft_ptr<T,false>
{
	// static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial ); // note: moved to dtor; see reasons there
	friend class owning_ptr<T,false>;
	T* t;

public:
	soft_ptr()
	{
		this->t = nullptr;
	}
	soft_ptr( owning_ptr<T,false>& owner )
	{
		this->t = owner.t;
	}
	soft_ptr( soft_ptr<T,false>& other )
	{
		this->t = other.t;
	}
	soft_ptr( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
	}

	soft_ptr& operator = ( soft_ptr<T,false>& other )
	{
		this->t = other.t;
		return *this;
	}
	soft_ptr& operator = ( soft_ptr<T,false>&& other )
	{
		this->t = other.t;
		other.t = nullptr;
		return *this;
	}

	void swap( soft_ptr<T, false>& other )
	{
		T* tmp = this->t;
		this->t = other.t;
		other.t = tmp;
	}


	T& operator * () const
	{
		return *t;
	}

	T* operator -> () const 
	{
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return this->t != nullptr;
	}

	~soft_ptr()
	{
		static_assert( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial );
	}
};
#endif // 0

template<class T, class T1, bool isSafe = NODECPP_ISSAFE_DEFAULT>
soft_ptr<T, isSafe> soft_ptr_static_cast( soft_ptr<T1, isSafe> p ) {
	soft_ptr<T, isSafe> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1, bool isSafe = NODECPP_ISSAFE_DEFAULT>
soft_ptr<T, isSafe> soft_ptr_reinterpret_cast( soft_ptr<T1, isSafe> p ) {
	soft_ptr<T, isSafe> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class naked_ptr
{
	friend class owning_ptr<T, isSafe>;
	friend class soft_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

	T* t;

public:
	naked_ptr() { t = nullptr; }

	naked_ptr(T& t_) { t = &t_; }

	template<class T1>
	naked_ptr( const owning_ptr<T1, isSafe>& owner ) { t = owner.get(); }
	naked_ptr( const owning_ptr<T, isSafe>& owner ) { t = owner.get(); }
	template<class T1>
	naked_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner ) { t = owner.get(); return *this; }
	naked_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner ) { t = owner.get(); return *this; }

	template<class T1>
	naked_ptr( const soft_ptr<T1, isSafe>& other ) { t = other.get(); }
	naked_ptr( const soft_ptr<T, isSafe>& other ) { t = other.get(); }
	template<class T1>
	naked_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other ) { t = other.get(); return *this; }
	naked_ptr<T>& operator = ( const soft_ptr<T, isSafe>& other ) { t = other.get(); return *this; }

	template<class T1>
	naked_ptr( const naked_ptr<T1, isSafe>& other ) { t = other.t; }
	template<class T1>
	naked_ptr<T>& operator = ( const naked_ptr<T1, isSafe>& other ) { t = other.t; return *this; }
	naked_ptr( const naked_ptr<T, isSafe>& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>& other ) = default;

	naked_ptr( naked_ptr<T, isSafe>&& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>&& other ) = default;

	void swap( naked_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
	}

	T& operator * () const
	{
		checkNotNullAllSizes( t );
		return *t;
	}

	T* operator -> () const 
	{
		checkNotNullLargeSize( t );
		return t;
	}

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t != nullptr;
	}

	bool operator == ( const naked_ptr<T, isSafe>& other ) const { return t == other.t; } // TODO: revise necessity

	~naked_ptr()
	{
		t = nullptr;
	}
};


#include "startup_checks.h"


#endif
