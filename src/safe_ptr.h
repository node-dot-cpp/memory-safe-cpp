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

#include <foundation.h>

namespace nodecpp::safememory
{
	constexpr uint64_t module_id = 2;
} // namespace nodecpp::safememory

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
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr );
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
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree != nullptr );
			PtrWishFlagsForSoftPtrList* tmp = ((PtrWishFlagsForSoftPtrList*)(firstFree->getPtr()));
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !firstFree->isUsed() );
			size_t idx;
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !firstFree->is1stBlock() );
			idx = firstFree - slots;
			firstFree->setPtr(ptr);
			firstFree->setUsed();
			firstFree = tmp;
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx < (1<<19) ); // TODO
				//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "at 2nd block 0x{:x}: inserted idx {} with 0x{:x}", (size_t)this, idx, (size_t)ptr);
			return idx;
		}
		void resetPtr( size_t idx, void* newPtr ) {
				//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "at 2nd block 0x{:x}: about to reset idx {} to 0x{:x}", (size_t)this, idx, (size_t)newPtr);
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx < otherAllockedCnt );
			slots[idx].setPtr( newPtr );
			slots[idx].setUsed();
			slots[idx].set2ndBlock();
			//dbgCheckFreeList();
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr || !firstFree->isUsed() );
		}
		void remove( size_t idx ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr || !firstFree->isUsed() );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx < otherAllockedCnt );
			slots[idx].setPtr( firstFree );
			firstFree = slots + idx;
			firstFree->setUnused();
			firstFree->set2ndBlock();
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr || !firstFree->isUsed() );
			//dbgCheckFreeList();
		}
		static SecondCBHeader* reallocate(SecondCBHeader* present )
		{
			if ( present != nullptr ) {
				NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, present->otherAllockedCnt != 0 );
				size_t newSize = (present->otherAllockedCnt << 1) + 2;
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (newSize + 2) * sizeof(PtrWishFlagsForSoftPtrList) ) );
				//PtrWishFlagsForSoftPtrList* newOtherAllockedSlots = 
				memcpy( ret->slots, present->slots, sizeof(PtrWishFlagsForSoftPtrList) * present->otherAllockedCnt );
				deallocate( present );
				//otherAllockedSlots.setPtr( newOtherAllockedSlots );
				ret->addToFreeList( ret->slots + present->otherAllockedCnt, newSize - present->otherAllockedCnt );
				ret->otherAllockedCnt = newSize;
				//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "after 2nd block relocation: ret = 0x{:x}, ret->otherAllockedCnt = {} (reallocation)", (size_t)ret, ret->otherAllockedCnt );
				return ret;
			}
			else {
				//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedCnt == 0 );
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (secondBlockStartSize + 2) * sizeof(PtrWishFlagsForSoftPtrList) ) );
				ret->otherAllockedCnt = secondBlockStartSize;
				//present->firstFree->set(nullptr);
				//otherAllockedSlots.setPtr( reinterpret_cast<PtrWishFlagsForSoftPtrList*>( allocate( otherAllockedCnt * sizeof(PtrWishFlagsForSoftPtrList) ) ) );
				ret->addToFreeList( ret->slots, secondBlockStartSize );
				//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "after 2nd block relocation: ret = 0x{:x}, ret->otherAllockedCnt = {} (ini allocation)", (size_t)ret, ret->otherAllockedCnt );
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
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !start->isUsed() );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ( start->getPtr() == 0 || start->is1stBlock() && (size_t)(start - slots) < maxSlots ) || ( (!start->is1stBlock()) && (size_t)(start - otherAllockedSlots.getPtr()) < otherAllockedCnt ) );
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
		//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !firstFree->isUsed() );
		dbgCheckFreeList();
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB initialized at 0x{:x}, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
	}
	/*void deinit() {
		if ( otherAllockedSlots.getPtr() != nullptr ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedCnt != 0 );
			//delete [] otherAllockedSlots;
			deallocate( otherAllockedSlots.getPtr() );
			otherAllockedCnt = 0;
		}
		else {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedCnt == 0 );
		}
	}*/
	void addToFreeList( PtrWishFlagsForSoftPtrList* begin, size_t count ) {
		//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr );
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
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: inserted 0x{:x} at idx {}", (size_t)this, (size_t)ptr, i );
					return i;
				}
		}
		//else
		{
			if ( otherAllockedSlots.getPtr() == nullptr || otherAllockedSlots.getPtr()->firstFree == nullptr )
			{
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: about to reset 2nd block, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
				otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: after reset 2nd block, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
			}
			assert ( otherAllockedSlots.getPtr() && otherAllockedSlots.getPtr()->firstFree );
			size_t idx = maxSlots + otherAllockedSlots.getPtr()->insert( ptr );
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: inserted 0x{:x} at idx {}", (size_t)this, (size_t)ptr, idx );
			return idx;
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: about to reset to 0x{:x} at idx {}", (size_t)this, (size_t)newPtr, idx );
		if ( idx < maxSlots ) {
			slots[idx].setPtr( newPtr );
			slots[idx].setUsed();
			slots[idx].set1stBlock();
		}
		else {
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			otherAllockedSlots.getPtr()->resetPtr( idx, newPtr );
		}
		//dbgCheckFreeList();
	}
	void remove( size_t idx ) {
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: about to remove at idx {}", (size_t)this, idx );
		if ( idx < maxSlots ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, slots[idx].isUsed() );
			slots[idx].setUnused();
			slots[idx].set1stBlock();
			otherAllockedSlots.setMask( otherAllockedSlots.getMask() & ~(1<<idx) );
		}
		else {
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx - maxSlots < otherAllockedCnt );
			idx -= maxSlots;
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedSlots.getPtr() != nullptr );
			otherAllockedSlots.getPtr()->remove( idx );
		}
		//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, firstFree == nullptr || !firstFree->isUsed() );
		//dbgCheckFreeList();
	}
	void clear() {
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: clear(), otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
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
	friend class owning_ptr;
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
		if ( this == &other ) return *this;
		t = other.t;
		other.t = nullptr;
		return *this;
	}
	template<class T1>
	owning_ptr( owning_ptr<T1, isSafe>&& other )
	{
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
	}
	template<class T1>
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t; // implicit cast, if at all possible
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
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, t == nullptr );
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



#define NODECPP_SAFE_PTR_DEBUG_MODE
#define NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
extern thread_local size_t onStackSafePtrCreationCount; 
extern thread_local size_t onStackSafePtrDestructionCount;
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {++onStackSafePtrCreationCount;}
//#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {if ( bornOnStack ) { /*NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, nodecpp::platform::is_guaranteed_on_stack( this ) );*/ ++onStackSafePtrDestructionCount;}}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() { if ( isOnStack() ) {++onStackSafePtrDestructionCount;} }
#else
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {}
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING


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

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::reference_impl__allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
#else
	using PointersT = nodecpp::platform::reference_impl__allocated_ptr_and_ptr_and_data_and_flags<26,1>; 
#endif
#else
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
#else
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<26,1>; 
#endif
#endif // SAFE_PTR_DEBUG_MODE
	PointersT pointers;
	T* getDereferencablePtr() const { return reinterpret_cast<T*>( pointers.get_ptr() ); }
	void* getAllocatedPtr() const {return pointers.get_allocated_ptr(); }
	void init( T* ptr, T* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }
	template<class T1>
	void init( T* ptr, T1* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }

	void invalidatePtr() { pointers.set_ptr(nullptr); pointers.set_allocated_ptr(nullptr); pointers.set_data(PointersT::max_data); }
	void setOnStack() { pointers.set_flag<0>(); }
	void setNotOnStack() { pointers.unset_flag<0>(); }
	bool isOnStack() { return pointers.has_flag<0>(); }

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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "2 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "3 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "4 created soft_ptr at 0x{:x}", (size_t)this );
	}
	template<class T1>
	soft_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other )
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "5 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "6 created soft_ptr at 0x{:x}", (size_t)this );
	}

	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>&& other )
	{
		// TODO+++: revise
		if ( this == &other ) return *this;
		bool wasOnStack = isOnStack();
		reset();
		if ( wasOnStack )
		{
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() == PointersT::max_data );
			if ( other.isOnStack() )
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				other.pointers.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				if ( other.getIdx_() != PointersT::max_data )
					other.getControlBlock()->remove(other.getIdx_());
				other.pointers.init( PointersT::max_data );
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
				other.pointers.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				pointers = other.pointers;
				if ( getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				other.pointers.init( PointersT::max_data );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "7 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "8 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "9 created soft_ptr at 0x{:x}", (size_t)this );
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
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "10 created soft_ptr at 0x{:x}", (size_t)this );
	}


	soft_ptr( T* t) // to beused for only types annotaded as [[nodecpp::owning_only]]
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			init( t, t, PointersT::max_data ); // automatic type conversion (if at all possible)
			setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( t )
				init( t, t, getControlBlock(t)->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t, t, PointersT::max_data ); // automatic type conversion (if at all possible)
	}

	void swap( soft_ptr<T, isSafe>& other )
	{
		bool iWasOnStack = isOnStack();
		bool otherWasOnStack = other.isOnStack();
		auto tmp = pointers;
		pointers = other.pointers;
		other.pointers = tmp;
		if ( iWasOnStack )
		{
			if ( otherWasOnStack )
			{
				assert(isOnStack());
				assert(other.isOnStack());
				// ... and we have to do nothing else
			}
			else
			{
				if ( getIdx_() != PointersT::max_data )
				{
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getDereferencablePtr() );
					getControlBlock()->remove(getIdx_());
				}
				setOnStack();
				pointers.set_data( PointersT::max_data );
				if ( other.getDereferencablePtr() )
					other.pointers.set_data( getControlBlock(other.getAllocatedPtr())->insert(&other) );
				else
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, other.getIdx_() == PointersT::max_data );
				other.setNotOnStack();
			}
		}
		else
		{
			if ( otherWasOnStack )
			{
				if ( getDereferencablePtr() )
					pointers.set_data( getControlBlock(getAllocatedPtr())->insert(this) );
				else
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() == PointersT::max_data );
				setNotOnStack();
				if ( other.getIdx_() != PointersT::max_data )
				{
					getControlBlock(other.getAllocatedPtr())->remove(other.getIdx_());
					other.pointers.set_data( PointersT::max_data );
				}
				other.setOnStack();
			}
			else
			{
				if ( getDereferencablePtr() && getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				if ( other.getDereferencablePtr() && other.getIdx_() != PointersT::max_data)
					other.getControlBlock()->resetPtr(other.getIdx_(), &other);
			}
		}
	}

	naked_ptr<T, isSafe> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getDereferencablePtr() != nullptr );
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
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() != Ptr2PtrWishData::invalidData );
			if ( getIdx_() != PointersT::max_data )
				getControlBlock()->remove(getIdx_());
			invalidatePtr();
		}
	}

	~soft_ptr()
	{
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "about to delete soft_ptr at 0x{:x}", (size_t)this );
		INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT()
		if( getDereferencablePtr() != nullptr ) {
			//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() != Ptr2PtrWishData::invalidData );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getAllocatedPtr() );
			/*if(getIdx_()>=3 && getIdx_() != PointersT::max_data)
			{
			//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "getIdx_() in dtor: 0x{:x}", getIdx_() ); // otherAllockedSlots.getPtr()
			//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "getAllocatedPtr() in dtor: 0x{:x}", (size_t)(getAllocatedPtr()) ); // otherAllockedSlots.getPtr()
			//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "getControlBlock() in dtor: 0x{:x}", (size_t)(getControlBlock()) ); // 
			//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "getControlBlock()->otherAllockedSlots.getPtr() in dtor: 0x{:x}", (size_t)(getControlBlock()->otherAllockedSlots.getPtr()) ); // 
			//if ( getControlBlock()->otherAllockedSlots.getPtr() == 0 )
			//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "isOnStack() in dtor: {}", isOnStack() ? "YES :(" : "NO" ); // 
			}*/
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

template<>
class soft_ptr<void, true>
{
	static constexpr bool isSafe = true;
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	template<class TT, bool isSafe1>
	friend class soft_ptr;
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::reference_impl__allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
#else
	using PointersT = nodecpp::platform::reference_impl__allocated_ptr_and_ptr_and_data_and_flags<26,1>; 
#endif
#else
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
#else
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<26,1>; 
#endif
#endif // SAFE_PTR_DEBUG_MODE
	PointersT pointers;
	void* getDereferencablePtr() const { return reinterpret_cast<void*>( pointers.get_ptr() ); }
	void* getAllocatedPtr() const {return pointers.get_allocated_ptr(); }
	void init( void* ptr, void* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }
	template<class T1>
	void init( void* ptr, T1* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }

	void invalidatePtr() { pointers.set_ptr(nullptr); pointers.set_allocated_ptr(nullptr); pointers.set_data(PointersT::max_data); }
	void setOnStack() { pointers.set_flag<0>(); }
	void setNotOnStack() { pointers.unset_flag<0>(); }
	bool isOnStack() { return pointers.has_flag<0>(); }

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
	}
	template<class T1>
	soft_ptr<void>& operator = ( const owning_ptr<T1, isSafe>& owner )
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
	}
	template<class T1>
	soft_ptr<void>& operator = ( const soft_ptr<T1, isSafe>& other )
	{
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
	soft_ptr( const soft_ptr<void, isSafe>& other )
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
	}
	soft_ptr<void>& operator = ( soft_ptr<void, isSafe>& other )
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


	soft_ptr( soft_ptr<void, isSafe>&& other )
	{
		if ( this == &other ) return;
		bool otherOnStack = other.isOnStack();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
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
	}

	soft_ptr<void>& operator = ( soft_ptr<void, isSafe>&& other )
	{
		// TODO+++: revise
		if ( this == &other ) return *this;
		bool wasOnStack = isOnStack();
		reset();
		if ( wasOnStack )
		{
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() == PointersT::max_data );
			if ( other.isOnStack() )
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				other.pointers.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
				if ( other.getIdx_() != PointersT::max_data )
					other.getControlBlock()->remove(other.getIdx_());
				other.pointers.init( PointersT::max_data );
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
				other.pointers.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				pointers = other.pointers;
				if ( getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				other.pointers.init( PointersT::max_data );
			}
		}
		return *this;
	}

	void swap( soft_ptr<void, isSafe>& other )
	{
		bool iWasOnStack = isOnStack();
		bool otherWasOnStack = other.isOnStack();
		auto tmp = pointers;
		pointers = other.pointers;
		other.pointers = tmp;
		if ( iWasOnStack )
		{
			if ( otherWasOnStack )
			{
				assert(isOnStack());
				assert(other.isOnStack());
				// ... and we have to do nothing else
			}
			else
			{
				if ( getIdx_() != PointersT::max_data )
				{
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getDereferencablePtr() );
					getControlBlock()->remove(getIdx_());
				}
				setOnStack();
				pointers.set_data( PointersT::max_data );
				if ( other.getDereferencablePtr() )
					other.pointers.set_data( getControlBlock(other.getAllocatedPtr())->insert(&other) );
				else
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, other.getIdx_() == PointersT::max_data );
				other.setNotOnStack();
			}
		}
		else
		{
			if ( otherWasOnStack )
			{
				if ( getDereferencablePtr() )
					pointers.set_data( getControlBlock(getAllocatedPtr())->insert(this) );
				else
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() == PointersT::max_data );
				setNotOnStack();
				if ( other.getIdx_() != PointersT::max_data )
				{
					getControlBlock(other.getAllocatedPtr())->remove(other.getIdx_());
					other.pointers.set_data( PointersT::max_data );
				}
				other.setOnStack();
			}
			else
			{
				if ( getDereferencablePtr() && getIdx_() != PointersT::max_data )
					getControlBlock()->resetPtr(getIdx_(), this);
				if ( other.getDereferencablePtr() && other.getIdx_() != PointersT::max_data)
					other.getControlBlock()->resetPtr(other.getIdx_(), &other);
			}
		}
	}

	explicit operator bool() const noexcept
	{
		return getDereferencablePtr() != nullptr;
	}

	void reset()
	{
		if( getDereferencablePtr() != nullptr ) {
			if ( getIdx_() != PointersT::max_data )
				getControlBlock()->remove(getIdx_());
			invalidatePtr();
		}
	}

	~soft_ptr()
	{
		INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT()
		if( getDereferencablePtr() != nullptr ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getAllocatedPtr() );
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

	T* get_dereferencable() const 
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

} // namespace nodecpp::safememory

#endif
