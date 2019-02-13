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

#include "safe_ptr_common.h"

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

template<>
void checkNotNullLargeSize<void>( void* ptr )
{
	// do nothing
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

template<class T, bool isSafe> class soft_ptr_base; // forward declaration
template<class T, bool isSafe> class soft_ptr; // forward declaration
template<class T, bool isSafe> class naked_ptr_base; // forward declaration
template<class T, bool isSafe> class naked_ptr; // forward declaration
class soft_this_ptr; // forward declaration

struct FirstControlBlock // not reallocatable
{
	struct PtrWishFlagsForSoftPtrList : public nodecpp::platform::allocated_ptr_with_flags<2> {
	public:
		void setPtr( void* ptr_ ) { nodecpp::platform::allocated_ptr_with_flags<2>::init(ptr_); }
		void* getPtr() const { return nodecpp::platform::allocated_ptr_with_flags<2>::get_ptr(); }
		void setUsed() { set_flag<0>(); }
		void setUnused() { unset_flag<0>(); }
		bool isUsed() const { return has_flag<0>(); }
		void set1stBlock() { set_flag<1>(); }
		void set2ndBlock() { unset_flag<1>(); }
		bool is1stBlock() const { return has_flag<1>(); }
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
		const SecondCBHeader* getPtr() const { return reinterpret_cast<SecondCBHeader*>( nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::get_ptr() ); }
		uint32_t getMask() const { return (uint32_t)(nodecpp::platform::allocated_ptr_with_mask_and_flags<3,1>::get_mask()); }
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

#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
	template <class T, bool isSafe>
	void dbgCheckValidity() const
	{
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( slots[i].isUsed() )
			{
				//bool ok = reinterpret_cast<soft_ptr<T, isSafe>*>(slots[i].getPtr())->getIdx_() == i;
				NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
					(reinterpret_cast<soft_ptr<T, isSafe>*>(slots[i].getPtr())->getIdx_() == i), 
					"getIdx_() = {}, i = {}", reinterpret_cast<soft_ptr<T, isSafe>*>(slots[i].getPtr())->getIdx_(), i );
			}
		if ( otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( otherAllockedSlots.getPtr()->slots[i].isUsed() )
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
						(reinterpret_cast<soft_ptr<T, isSafe>*>(otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_() == i + FirstControlBlock::maxSlots), 
						"getIdx_() = {}, expected: {}", reinterpret_cast<soft_ptr<T, isSafe>*>(otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_(), i + FirstControlBlock::maxSlots );
	}
#else
	template <class T, bool isSafe>
	void dbgCheckValidity() const {}
#endif // NODECPP_SAFEMEMORY_HEAVY_DEBUG

#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
	void dbgCheckIdxConsistency( size_t idx, const void* ptr ) const {
		if ( idx < maxSlots ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, slots[idx].isUsed(), "idx = {}", idx );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, slots[idx].getPtr() == ptr, "idx = {}", idx );
		}
		else {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedSlots.getPtr() != nullptr );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx - maxSlots < otherAllockedSlots.getPtr()->otherAllockedCnt, "idx - maxSlots = {}, otherAllockedSlots.getPtr()->otherAllockedCnt = {}", idx - maxSlots, otherAllockedSlots.getPtr()->otherAllockedCnt);
			idx -= maxSlots;
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedSlots.getPtr()->slots[idx].isUsed(), "idx = {}", idx );
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedSlots.getPtr()->slots[idx].getPtr() == ptr, "idx = {}", idx );
		}
	}
#else
	void dbgCheckIdxConsistency( size_t idx, const void* ptr ) const {}
#endif // NODECPP_SAFEMEMORY_HEAVY_DEBUG

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
		dbgCheckValidity<void, true>();
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
		dbgCheckValidity<void, true>();
	}
	void enlargeSecondBlock() {
		otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
	}
	size_t insert( void* ptr ) {
		dbgCheckValidity<void, true>();
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
		dbgCheckValidity<void, true>();
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
		dbgCheckValidity<void, true>();
	}
	void clear() {
		//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "1CB 0x{:x}: clear(), otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
		if ( otherAllockedSlots.getPtr() != nullptr )
			otherAllockedSlots.getPtr()->dealloc();
		otherAllockedSlots.setZombie();
		//dbgCheckValidity<void, true>();
	}
	bool isZombie() { return otherAllockedSlots.isZombie(); }
};
//static_assert( sizeof(FirstControlBlock) == 32 );



inline
FirstControlBlock* getControlBlock_(void* t) { return reinterpret_cast<FirstControlBlock*>(t) - 1; }
inline
uint8_t* getAllocatedBlock_(void* t) { return reinterpret_cast<uint8_t*>(getControlBlock_(t)) + getPrefixByteCount(); }
inline
uint8_t* getAllocatedBlockFromControlBlock_(void* cb) { return reinterpret_cast<uint8_t*>(cb) + getPrefixByteCount(); }
inline
void* getPtrToAllocatedObjectFromControlBlock_( void* allocObjPtr ) { return (reinterpret_cast<FirstControlBlock*>(allocObjPtr)) + 1; }


struct make_owning_t {};
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
	friend class soft_ptr_base;
	template<class TT, bool isSafe1>
	friend class soft_ptr;

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
	using base_pointer_t = nodecpp::platform::ptrwithdatastructsdefs::generic_ptr_with_zombie_property_; 
#else
	using base_pointer_t = nodecpp::platform::ptr_with_zombie_property; 
#endif // SAFE_PTR_DEBUG_MODE
	template<class TT>
	struct ObjectPointer : base_pointer_t {
	public:
		void setPtr( void* ptr_ ) { base_pointer_t::init(ptr_); }
		void setTypedPtr( T* ptr_ ) { base_pointer_t::init(ptr_); }
		void* getPtr() const { return base_pointer_t::get_ptr(); }
		TT* getTypedPtr() const { return reinterpret_cast<TT*>( base_pointer_t::get_ptr() ); }
		void setZombie() { base_pointer_t::set_zombie(); }
		//bool isZombie() const { return base_pointer_t::is_sombie(); }
	};
	ObjectPointer<T> t; // the only data member!

	FirstControlBlock* getControlBlock() { return getControlBlock_(t.getPtr()); }
	const FirstControlBlock* getControlBlock() const { return getControlBlock_(t.getPtr()); }
	uint8_t* getAllocatedBlock() {return getAllocatedBlock_(t.getPtr()); }

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

#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
	void dbgCheckValidity() const
	{
		if ( t == nullptr )
			return;
		const FirstControlBlock* cb = getControlBlock();
		cb->dbgCheckValidity<T, isSafe>();
		/*for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
			{
				//bool ok = reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->getIdx_() == i;
				NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
					(reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->getIdx_() == i), 
					"getIdx_() = {}, i = {}", reinterpret_cast<soft_ptr<T, isSafe>*>(cb->slots[i].getPtr())->getIdx_(), i );
			}
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
						(reinterpret_cast<soft_ptr<T, isSafe>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_() == i + FirstControlBlock::maxSlots), 
						"getIdx_() = {}, expected: {}", reinterpret_cast<soft_ptr<T, isSafe>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_(), i + FirstControlBlock::maxSlots );*/
	}
#else
	void dbgCheckValidity() const {}
#endif // NODECPP_SAFEMEMORY_HEAVY_DEBUG

public:

	owning_ptr( make_owning_t, T* t_ ) // make it private with a friend make_owning()!
	{
		t.setPtr( t_ );
		getControlBlock()->init();
		dbgCheckValidity();
	}
	owning_ptr()
	{
		t.setPtr( nullptr );
	}
	owning_ptr( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr& operator = ( owning_ptr<T, isSafe>& other ) = delete;
	owning_ptr( owning_ptr<T, isSafe>&& other )
	{
		t.setTypedPtr( other.t.getTypedPtr() );
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
	}
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other )
	{
		if ( this == &other ) return *this;
		t.setTypedPtr( other.t.getTypedPtr() );
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
		return *this;
	}
	template<class T1>
	owning_ptr( owning_ptr<T1, isSafe>&& other )
	{
		t.setTypedPtr( other.t.getTypedPtr() ); // implicit cast, if at all possible
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
	}
	template<class T1>
	owning_ptr& operator = ( owning_ptr<T, isSafe>&& other )
	{
		if ( this == &other ) return *this;
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
		other.dbgCheckValidity();
		dbgCheckValidity();
		return *this;
	}
	~owning_ptr()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t.getTypedPtr()) )
		{
			updatePtrForListItemsWithInvalidPtr();
			destruct( t.getTypedPtr() );
			zombieDeallocate( getAllocatedBlock_(t.getTypedPtr()) );
			getControlBlock()->clear();
			t.setZombie();
			forcePreviousChangesToThisInDtor(this); // force compilers to apply the above instruction
		}
		dbgCheckValidity();
	}

	void reset()
	{
		if ( NODECPP_LIKELY(t.getTypedPtr()) )
		{
			updatePtrForListItemsWithInvalidPtr();
			destruct( t.getTypedPtr() );
			zombieDeallocate( getAllocatedBlock_(t.getTypedPtr()) );
			getControlBlock()->clear();
			t.setPtr( nullptr );
		}
		dbgCheckValidity();
	}

	/*void reset( T* t_ )
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, t == nullptr );
		reset();
	}*/

	void swap( owning_ptr<T, isSafe>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
		other.dbgCheckValidity();
		dbgCheckValidity();
	}

	naked_ptr<T, isSafe> get() const
	{
		naked_ptr<T, isSafe> ret;
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

	bool operator == (const soft_ptr<T, isSafe>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr<T1, isSafe1>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }

	bool operator != (const soft_ptr<T, isSafe>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr<T1, isSafe1>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() != nullptr; }

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t.getTypedPtr() != nullptr;
	}
};


extern thread_local void* thg_stackPtrForMakeOwningCall;

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	NODISCARD owning_ptr<_Ty> make_owning(_Types&&... _Args)
	{
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	uint8_t* dataForObj = data + sizeof(FirstControlBlock) - getPrefixByteCount();
	owning_ptr<_Ty> op(make_owning_t(), (_Ty*)(uintptr_t)(dataForObj));
	void* stackTmp = thg_stackPtrForMakeOwningCall;
	thg_stackPtrForMakeOwningCall = dataForObj;
	_Ty* objPtr = new ( dataForObj ) _Ty(::std::forward<_Types>(_Args)...);
	thg_stackPtrForMakeOwningCall = stackTmp;
	//return owning_ptr<_Ty>(objPtr);
	return op;
	}



#define NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
extern thread_local size_t onStackSafePtrCreationCount; 
extern thread_local size_t onStackSafePtrDestructionCount;
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {++onStackSafePtrCreationCount;}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() { if ( isOnStack() ) {++onStackSafePtrDestructionCount;} }
#else
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {}
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr_base
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	/*friend class naked_ptr_base<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class naked_ptr_base;
	friend class naked_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class naked_ptr;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	friend struct FirstControlBlock;

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::ptrwithdatastructsdefs::generic_struct_allocated_ptr_and_ptr_and_data_and_flags_<32,1>; 
#else
	using PointersT = nodecpp::platform::ptrwithdatastructsdefs::generic_struct_allocated_ptr_and_ptr_and_data_and_flags_<26,1>; 
#endif
#else
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<32,1>; 
#else
	using PointersT = nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<26,1>; 
#endif
#endif // SAFE_PTR_DEBUG_MODE
	template<class TT>
	struct Pointers : public PointersT {
		T* getDereferencablePtr() const { return reinterpret_cast<T*>( get_ptr() ); }
		void* getAllocatedPtr() const {return get_allocated_ptr(); }
		void init( size_t data ) { PointersT::init( data ); }
		void init( T* ptr, T* allocptr, size_t data ) { PointersT::init( ptr, allocptr, data ); }
		template<class T1>
		void init( T* ptr, T1* allocptr, size_t data ) { PointersT::init( ptr, allocptr, data ); }

		void invalidatePtr() { PointersT::init(PointersT::max_data); }
		void setPtrZombie() { set_data(PointersT::max_data); set_zombie(); }
		void setOnStack() { set_flag<0>(); }
		void setNotOnStack() { unset_flag<0>(); }
		bool isOnStack() { return has_flag<0>(); }
	};
	Pointers<T> pointers; // the only data member!

	T* getDereferencablePtr() const { return reinterpret_cast<T*>( pointers.get_ptr() ); }
	void* getAllocatedPtr() const {return pointers.get_allocated_ptr(); }
	void init( size_t data ) { pointers.init( data ); }
	void init( T* ptr, T* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }
	void initOnStack( T* ptr, T* allocptr ) { pointers.init( ptr, allocptr, PointersT::max_data ); setOnStack(); }
	template<class T1>
	void init( T* ptr, T1* allocptr, size_t data ) { pointers.init( ptr, allocptr, data ); }
	template<class T1>
	void initOnStack( T* ptr, T1* allocptr ) { pointers.init( ptr, allocptr, PointersT::max_data ); setOnStack(); }

	void invalidatePtr() { pointers.invalidatePtr(); }
	void setPtrZombie() { pointers.setPtrZombie(); }
	void setOnStack() { pointers.setOnStack(); }
	void setNotOnStack() { pointers.setNotOnStack(); }
	bool isOnStack() { return pointers.isOnStack(); }

	size_t getIdx_() const { return pointers.get_data(); }
	void setIdx_( size_t idx ) { pointers.set_data( idx ); }
	FirstControlBlock* getControlBlock() const { return getControlBlock_(pointers.getAllocatedPtr()); }
	static FirstControlBlock* getControlBlock(void* t) { return getControlBlock_(t); }

	soft_ptr_base(FirstControlBlock* cb, T* t) // to be used for only types annotaded as [[nodecpp::owning_only]]
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, cb != nullptr );
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( t, getPtrToAllocatedObjectFromControlBlock_(cb) ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( t )
				init( t, getPtrToAllocatedObjectFromControlBlock_(cb), cb->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t, getPtrToAllocatedObjectFromControlBlock_(cb), PointersT::max_data ); // automatic type conversion (if at all possible)
		dbgCheckMySlotConsistency();
	}

public:
#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
	void dbgCheckMySlotConsistency() const
	{
		if ( getIdx_() != PointersT::max_data )
		{
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getAllocatedPtr() != nullptr );
			getControlBlock()->dbgCheckIdxConsistency(getIdx_(), this);
		}
	}
#else
	void dbgCheckMySlotConsistency() const {}
#endif // NODECPP_SAFEMEMORY_HEAVY_DEBUG

	soft_ptr_base() {}

	template<class T1>
	soft_ptr_base( const owning_ptr<T1, isSafe>& owner )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		dbgCheckMySlotConsistency();
	}
	template<class T1>
	soft_ptr_base<T>& operator = ( const owning_ptr<T1, isSafe>& owner )
	{
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
		}
		else
			if ( owner.t .getPtr())
				init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		dbgCheckMySlotConsistency();
		return *this;
	}


	template<class T1>
	soft_ptr_base( const soft_ptr_base<T1, isSafe>& other )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}
	template<class T1>
	soft_ptr_base<T>& operator = ( const soft_ptr_base<T1, isSafe>& other )
	{
		//if ( this == &other ) return *this;
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
		return *this;
	}
	soft_ptr_base( const soft_ptr_base<T, isSafe>& other )
	{
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}
	soft_ptr_base<T>& operator = ( soft_ptr_base<T, isSafe>& other )
	{
		if ( this == &other ) return *this;
		bool iWasOnStack = isOnStack();
		reset();
		if ( iWasOnStack )
		{
			initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
		}
		else
			if ( other.getAllocatedPtr() )
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( other.getDereferencablePtr(), other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
		return *this;
	}


	soft_ptr_base( soft_ptr_base<T, isSafe>&& other )
	{
		if ( this == &other ) return;
		bool otherOnStack = other.isOnStack();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
			if ( other.getIdx_() != PointersT::max_data )
				other.getControlBlock()->remove(other.getIdx_());
			other.init(PointersT::max_data);
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
				other.init(PointersT::max_data);
				other.setOnStack();
			}
			else
			{
				pointers = other.pointers;
				if ( other.getDereferencablePtr() )
					getControlBlock(getAllocatedPtr())->resetPtr(getIdx_(), this);
				other.init(PointersT::max_data);
			}
		}
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}

	soft_ptr_base<T>& operator = ( soft_ptr_base<T, isSafe>&& other )
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
				initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
				other.init( PointersT::max_data );
				other.setOnStack();
			}
			else
			{
				initOnStack( other.getDereferencablePtr(), other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
				if ( other.getIdx_() != PointersT::max_data )
					other.getControlBlock()->remove(other.getIdx_());
				other.init( PointersT::max_data );
			}
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
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
		return *this;
	}

	template<class T1>
	soft_ptr_base( const owning_ptr<T1, isSafe>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( t_, owner.t.getPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t.getPtr())
				init( t_, owner.t.getPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t.getPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		dbgCheckMySlotConsistency();
	}

	template<class T1>
	soft_ptr_base( const soft_ptr_base<T1, isSafe>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( t_, other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( t_, other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}
	soft_ptr_base( const soft_ptr_base<T, isSafe>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			initOnStack( t_, other.getAllocatedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( other.getAllocatedPtr() )
				init( t_, other.getAllocatedPtr(), getControlBlock(other.getAllocatedPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, other.getAllocatedPtr(), PointersT::max_data ); // automatic type conversion (if at all possible)
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}

	void swap( soft_ptr_base<T, isSafe>& other )
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
				setIdx_( PointersT::max_data );
				if ( other.getDereferencablePtr() )
					other.setIdx_( getControlBlock(other.getAllocatedPtr())->insert(&other) );
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
					setIdx_( getControlBlock(getAllocatedPtr())->insert(this) );
				else
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getIdx_() == PointersT::max_data );
				setNotOnStack();
				if ( other.getIdx_() != PointersT::max_data )
				{
					getControlBlock(other.getAllocatedPtr())->remove(other.getIdx_());
					other.setIdx_( PointersT::max_data );
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
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}

	naked_ptr<T, isSafe> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getDereferencablePtr() != nullptr );
		naked_ptr<T, isSafe> ret;
		ret.t = getDereferencablePtr();
		return ret;
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
		dbgCheckMySlotConsistency();
	}

	bool operator == (const owning_ptr<T, isSafe>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr<T1, isSafe>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_base<T, isSafe>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr_base<T1, isSafe1>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr<T, isSafe>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr<T1, isSafe>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_base<T, isSafe>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr_base<T1, isSafe1>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() != nullptr; }

	~soft_ptr_base()
	{
		dbgCheckMySlotConsistency();
		INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT()
		if( getDereferencablePtr() != nullptr ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getAllocatedPtr() );
			if ( getIdx_() != PointersT::max_data )
			{
				assert(!isOnStack());
				getControlBlock()->remove(getIdx_());
			}
			setPtrZombie();
			forcePreviousChangesToThisInDtor(this); // force compilers to apply the above instruction
		}
	}
};

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class soft_ptr : public soft_ptr_base<T, isSafe>
{
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	friend class owning_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;

	/*friend class naked_ptr_base<T, isSafe>;
	friend class naked_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class naked_ptr;
	template<class TT, bool isSafe1>
	friend class naked_ptr_base;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	friend struct FirstControlBlock;

private:
	friend class soft_this_ptr;
	template<class TT>
	friend soft_ptr<TT> soft_ptr_in_constructor(TT* ptr);
	friend soft_ptr<T> soft_ptr_in_constructor(T* ptr);
	soft_ptr(FirstControlBlock* cb, T* t) : soft_ptr_base<T, isSafe>(cb, t) {} // to be used for only types annotaded as [[nodecpp::owning_only]]

public:
	soft_ptr() : soft_ptr_base<T, isSafe>()
	{
		this->init( soft_ptr_base<T, isSafe>::PointersT::max_data );
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}


	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner ) : soft_ptr_base<T, isSafe>(owner) {}
	soft_ptr( const owning_ptr<T, isSafe>& owner )
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
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base<T, isSafe>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}
	template<class T1>
	soft_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner )
	{
		soft_ptr_base<T, isSafe>::operator = (owner);
		return *this;
	}
	soft_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner )
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
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base<T, isSafe>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
		return *this;
	}


	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other ) : soft_ptr_base<T, isSafe>(other) {}
	template<class T1>
	soft_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other ) : soft_ptr_base<T, isSafe>(other) {}
	soft_ptr( const soft_ptr<T, isSafe>& other ) : soft_ptr_base<T, isSafe>(other) {}
	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>& other )
	{
		soft_ptr_base<T, isSafe>::operator = (other);
		return *this;
	}


	soft_ptr( soft_ptr<T, isSafe>&& other ) : soft_ptr_base<T, isSafe>( std::move(other) ) {}

	soft_ptr<T>& operator = ( soft_ptr<T, isSafe>&& other )
	{
		soft_ptr_base<T, isSafe>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner, T* t_ ) : soft_ptr_base<T, isSafe>(owner, t_) {}
	soft_ptr( const owning_ptr<T, isSafe>& owner, T* t_ )
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
	soft_ptr( const soft_ptr<T1, isSafe>& other, T* t_ ) : soft_ptr_base<T, isSafe>(other, t_) {}
	soft_ptr( const soft_ptr<T, isSafe>& other, T* t_ ) : soft_ptr_base<T, isSafe>(other, t_) {}

	void swap( soft_ptr<T, isSafe>& other )
	{
		soft_ptr_base<T, isSafe>::swap(other);
	}

	naked_ptr<T, isSafe> get() const
	{
		return soft_ptr_base<T, isSafe>::get();
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
		soft_ptr_base<T, isSafe>::reset();
	}

	bool operator == (const owning_ptr<T, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr<T1, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr<T, isSafe>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr<T1, isSafe1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr<T, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr<T1, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr<T, isSafe>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr<T1, isSafe1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }
};

template<>
class soft_ptr<void, true> : public soft_ptr_base<void, true>
{
	static constexpr bool isSafe = true;
//	static_assert( ( (!isSafe) && ( NODECPP_ISSAFE_MODE == MemorySafety::none || NODECPP_ISSAFE_MODE == MemorySafety::partial) ) || ( isSafe && ( NODECPP_ISSAFE_MODE == MemorySafety::full || NODECPP_ISSAFE_MODE == MemorySafety::partial) ));
	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;
	template<class TT, bool isSafe1>
	friend class soft_ptr;

	/*friend class naked_ptr_base<void, true>;
	friend class naked_ptr<void, true>;
	template<class TT, bool isSafe1>
	friend class naked_ptr;
	template<class TT, bool isSafe1>
	friend class naked_ptr_base;*/

	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_static_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );
	template<class TT, class TT1>
	friend soft_ptr<TT, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast( soft_ptr<TT1, NODECPP_ISSAFE_DEFAULT> );
	friend struct FirstControlBlock;
	template<class TT, class TT1, bool isSafe1>
	friend soft_ptr<TT, isSafe1> soft_ptr_reinterpret_cast( soft_ptr<TT1, isSafe1> );
	friend struct FirstControlBlock;

public:
	soft_ptr() : soft_ptr_base<void, isSafe>()
	{
		pointers.init( soft_ptr_base<void, isSafe>::PointersT::max_data );
		if ( nodecpp::platform::is_guaranteed_on_stack( this ) )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}


	template<class T1>
	soft_ptr( const owning_ptr<T1, isSafe>& owner ) : soft_ptr_base<void, isSafe>(owner) {}
	template<class T1>
	soft_ptr<void>& operator = ( const owning_ptr<T1, isSafe>& owner )
	{
		soft_ptr_base<void, isSafe>::operator = (owner);
		return *this;
	}


	template<class T1>
	soft_ptr( const soft_ptr<T1, isSafe>& other ) : soft_ptr_base<void, isSafe>( other ) {}
	template<class T1>
	soft_ptr<void>& operator = ( const soft_ptr<T1, isSafe>& other )
	{
		soft_ptr_base<void, isSafe>::operator = (other);
		return *this;
	}
	soft_ptr( const soft_ptr<void, isSafe>& other ) : soft_ptr_base<void, isSafe>( other ) {}
	soft_ptr<void>& operator = ( soft_ptr<void, isSafe>& other )
	{
		soft_ptr_base<void, isSafe>::operator = (other);
		return *this;
	}


	soft_ptr( soft_ptr<void, isSafe>&& other ) : soft_ptr_base<void, isSafe>(  std::move(other)  ) {}

	soft_ptr<void>& operator = ( soft_ptr<void, isSafe>&& other )
	{
		soft_ptr_base<void, isSafe>::operator = ( std::move(other) );
		return *this;
	}

	void swap( soft_ptr<void, isSafe>& other )
	{
		soft_ptr_base<void, isSafe>::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->getDereferencablePtr() != nullptr;
	}

	template<class T1, bool isSafe1> 
	bool operator == (const owning_ptr<T1, isSafe>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr<void, isSafe>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator == (const soft_ptr<T1, isSafe1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	template<class T1, bool isSafe1> 
	bool operator != (const owning_ptr<T1, isSafe>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr<void, isSafe>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1, bool isSafe1>
	bool operator != (const soft_ptr<T1, isSafe1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }

	void reset()
	{
		soft_ptr_base<void, isSafe>::reset();
	}
};


template<class T, class T1, bool isSafe>
soft_ptr<T, isSafe> soft_ptr_static_cast( soft_ptr<T1, isSafe> p ) {
	soft_ptr<T, isSafe> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1>
soft_ptr<T, NODECPP_ISSAFE_DEFAULT> soft_ptr_static_cast( soft_ptr<T1, NODECPP_ISSAFE_DEFAULT> p ) {
	soft_ptr<T, NODECPP_ISSAFE_DEFAULT> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1, bool isSafe>
soft_ptr<T, isSafe> soft_ptr_reinterpret_cast( soft_ptr<T1, isSafe> p ) {
	soft_ptr<T, isSafe> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1>
soft_ptr<T, NODECPP_ISSAFE_DEFAULT> soft_ptr_reinterpret_cast( soft_ptr<T1, NODECPP_ISSAFE_DEFAULT> p ) {
	soft_ptr<T, NODECPP_ISSAFE_DEFAULT> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}


class soft_this_ptr
{
	FirstControlBlock* cbPtr = nullptr;
	uint32_t offset;

public:
	soft_this_ptr()
	{
		cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
		uintptr_t delta = reinterpret_cast<uint8_t*>(this) - reinterpret_cast<uint8_t*>(cbPtr);
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, delta <= UINT32_MAX, "delta = 0x{:x}", delta );
		offset = (uint32_t)delta;
	}

	soft_this_ptr( soft_this_ptr& other ) {}
	soft_this_ptr& operator = ( soft_this_ptr& other ) { return *this; }

	soft_this_ptr( soft_this_ptr&& other ) {}
	soft_this_ptr& operator = ( soft_this_ptr&& other ) { return *this; }

	explicit operator bool() const noexcept
	{
		return cbPtr != nullptr;
	}

	template<class T>
	soft_ptr<T> getSoftPtr(T* ptr)
	{
		void* allocatedPtr = getAllocatedBlockFromControlBlock_( getAllocatedBlock_(cbPtr) );
		if ( allocatedPtr == nullptr )
			throwPointerOutOfRange();
		//return soft_ptr<T, true>( allocatedPtr, ptr );
		//FirstControlBlock* cb = cbPtr;
		FirstControlBlock* cb = reinterpret_cast<FirstControlBlock*>( reinterpret_cast<uint8_t*>(this) - offset );
		return soft_ptr<T, true>( cb, ptr );
	}

	~soft_this_ptr()
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

template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class naked_ptr_base
{
	friend class owning_ptr<T, isSafe>;
	friend class soft_ptr_base<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;
	//friend class soft_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr;

	template<class TT, bool isSafe1>
	friend class naked_ptr_base;
	friend class naked_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class naked_ptr;

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

	T* t;

public:
	naked_ptr_base() { t = nullptr; }

	template<class T1>
	naked_ptr_base( const owning_ptr<T1, isSafe>& owner ) { *this = owner.get(); }

	template<class T1>
	naked_ptr_base<T>& operator = ( const owning_ptr<T1, isSafe>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	naked_ptr_base( const soft_ptr<T1, isSafe>& other ) { *this = other.get(); }
	naked_ptr_base( const soft_ptr<T, isSafe>& other ) { *this = other.get(); }
	template<class T1>
	naked_ptr_base<T>& operator = ( const soft_ptr<T1, isSafe>& other ) { *this = other.get(); return *this; }
	naked_ptr_base<T>& operator = ( const soft_ptr<T, isSafe>& other ) { *this = other.get(); return *this; }

	template<class T1>
	naked_ptr_base( const naked_ptr_base<T1, isSafe>& other ) { t = other.t; }
	template<class T1>
	naked_ptr_base<T>& operator = ( const naked_ptr_base<T1, isSafe>& other ) { t = other.t; return *this; }
	naked_ptr_base( const naked_ptr_base<T, isSafe>& other ) = default;
	naked_ptr_base<T, isSafe>& operator = ( naked_ptr_base<T, isSafe>& other ) = default;

	naked_ptr_base( naked_ptr_base<T, isSafe>&& other ) = default;
	naked_ptr_base<T, isSafe>& operator = ( naked_ptr_base<T, isSafe>&& other ) = default;

	void swap( naked_ptr_base<T, isSafe>& other )
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


	bool operator == ( const naked_ptr_base<T, isSafe>& other ) const { return t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const naked_ptr_base<T1, isSafe1>& other ) const { return t == other.t; }

	bool operator != ( const naked_ptr_base<T, isSafe>& other ) const { return t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const naked_ptr_base<T1, isSafe1>& other ) const { return t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return t != nullptr; }

	~naked_ptr_base()
	{
		t = nullptr;
	}
};


template<class T, bool isSafe = NODECPP_ISSAFE_DEFAULT>
class naked_ptr : public naked_ptr_base<T, isSafe>
{
	friend class owning_ptr<T, isSafe>;
	friend class soft_ptr_base<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;
	//friend class soft_ptr<T, isSafe>;
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr;

	static_assert( isSafe ); // note: some compilers may check this even if this default specialization is not instantiated; if so, switch to the commented line above

public:
	naked_ptr() : naked_ptr_base<T, isSafe>() {}

	naked_ptr(T& t_) : naked_ptr_base(t_) { this->t = &t_; }

	template<class T1>
	naked_ptr( const owning_ptr<T1, isSafe>& owner ) : naked_ptr_base<T, isSafe>(owner) {}
	naked_ptr( const owning_ptr<T, isSafe>& owner ) : naked_ptr_base<T, isSafe>() {*this = owner.get();}
	template<class T1>
	naked_ptr<T>& operator = ( const owning_ptr<T1, isSafe>& owner ) { *this = owner.get(); return *this; }
	naked_ptr<T>& operator = ( const owning_ptr<T, isSafe>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	naked_ptr( const soft_ptr<T1, isSafe>& other ) : naked_ptr_base<T, isSafe>(other) {}
	naked_ptr( const soft_ptr<T, isSafe>& other ) : naked_ptr_base<T, isSafe>(other) {}
	template<class T1>
	naked_ptr<T>& operator = ( const soft_ptr<T1, isSafe>& other ) { *this = other.get(); return *this; }
	naked_ptr<T>& operator = ( const soft_ptr<T, isSafe>& other ) { *this = other.get(); return *this; }

	template<class T1>
	naked_ptr( const naked_ptr<T1, isSafe>& other ) : naked_ptr_base<T, isSafe>(other) {}
	template<class T1>
	naked_ptr<T>& operator = ( const naked_ptr<T1, isSafe>& other ) { this->t = other.t; return *this; }
	naked_ptr( const naked_ptr<T, isSafe>& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>& other ) = default;

	naked_ptr( naked_ptr<T, isSafe>&& other ) = default;
	naked_ptr<T, isSafe>& operator = ( naked_ptr<T, isSafe>&& other ) = default;

	void swap( naked_ptr<T, isSafe>& other )
	{
		naked_ptr_base( other );
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

	bool operator == ( const naked_ptr<T, isSafe>& other ) const { return this->t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const naked_ptr<T1, isSafe1>& other ) const { return this->t == other.t; }

	bool operator != ( const naked_ptr<T, isSafe>& other ) const { return this->t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const naked_ptr<T1, isSafe1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~naked_ptr()
	{
		this->t = nullptr;
	}
};


template<>
class naked_ptr<void, true> : public naked_ptr_base<void, true>
{
	template<class TT, bool isSafe1>
	friend class owning_ptr;
	friend class soft_ptr_base<void, true>;
	template<class TT, bool isSafe1>
	friend class soft_ptr_base;
	//friend class soft_ptr<void, true>;
	//template<class TT, bool isSafe1>
	//friend class soft_ptr;

public:
	naked_ptr() : naked_ptr_base() {}

	template<class T1>
	naked_ptr(T1& t_) : naked_ptr_base<void>(t_) {}

	template<class T1>
	naked_ptr( const owning_ptr<T1, true>& owner ) : naked_ptr_base(owner) {}
	template<class T1>
	naked_ptr<void>& operator = ( const owning_ptr<T1, true>& owner ) { *this = owner.get(); return *this; }

	template<class T1>
	naked_ptr( const soft_ptr<T1, true>& other ) : naked_ptr_base(other) {}
	naked_ptr( const soft_ptr<void, true>& other ) : naked_ptr_base(other) {}
	template<class T1>
	naked_ptr<void>& operator = ( const soft_ptr<T1, true>& other ) { *this = other.get(); return *this; }
	naked_ptr<void>& operator = ( const soft_ptr<void, true>& other ) { *this = other.get(); return *this; }

	template<class T1>
	naked_ptr( const naked_ptr<T1, true>& other ) : naked_ptr_base(other) {}
	template<class T1>
	naked_ptr<void>& operator = ( const naked_ptr<T1, true>& other ) { t = other.t; return *this; }
	naked_ptr( const naked_ptr<void, true>& other ) = default;
	naked_ptr<void, true>& operator = ( naked_ptr<void, true>& other ) = default;

	naked_ptr( naked_ptr<void, true>&& other ) = default;
	naked_ptr<void, true>& operator = ( naked_ptr<void, true>&& other ) = default;

	void swap( naked_ptr<void, true>& other )
	{
		naked_ptr_base::swap( other );
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

	bool operator == ( const naked_ptr<void, true>& other ) const { return this->t == other.t; }
	template<class T1, bool isSafe1>
	bool operator == ( const naked_ptr<T1, isSafe1>& other ) const { return this->t == other.t; }

	bool operator != ( const naked_ptr<void, true>& other ) const { return this->t != other.t; }
	template<class T1, bool isSafe1>
	bool operator != ( const naked_ptr<T1, isSafe1>& other ) const { return this->t != other.t; }

	bool operator == (std::nullptr_t nullp ) const { return this->t == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { return this->t != nullptr; }

	~naked_ptr()
	{
		this->t = nullptr;
	}
};


} // namespace nodecpp::safememory

#include "startup_checks.h"

#endif
