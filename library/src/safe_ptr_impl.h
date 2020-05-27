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
#include <stack_info.h>
#include "../include/nodecpp_error/nodecpp_error.h"

namespace nodecpp::safememory
{

extern thread_local void* thg_stackPtrForMakeOwningCall;

template<class T>
void checkNotNullLargeSize( T* ptr )
{
#ifdef NODECPP_WINDOWS
	if constexpr ( !std::is_same<T, void>::value )
	{
		if constexpr ( sizeof(T) <= NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE ) ;
		else {
			if ( ptr == nullptr )
				throw ::nodecpp::error::zero_pointer_access;
		}
	}
#else
	// due to problems with signal handling while LTO is enabled (both clang and gcc) we cannot rely on respective code on Linux
	if ( ptr == nullptr )
		throw ::nodecpp::error::zero_pointer_access;
#endif
}

inline
void checkNotNullLargeSize( void* )
{
	// do nothing
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, false, "not expected to be called" );
}

template<class T>
void checkNotNullAllSizes( T* ptr )
{
	if ( ptr == nullptr )
		throw ::nodecpp::error::zero_pointer_access;
}

inline
void throwPointerOutOfRange()
{
	// TODO: actual implementation
	throw std::bad_alloc();
}


#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
struct DbgCreationInfo
{
	enum Origination { inizero=0, reset=1, created=2, movedin=3, movedout=4 };
	Origination origination = Origination::inizero;
//	static constexpr const char* originationStr[] = { "", "destructing its owning_ptr<>", "resetting its own_ptr<>", "nulling its owning_ptr<>" };
	nodecpp::StackInfo creationPoint;

	DbgCreationInfo() {}
	DbgCreationInfo( const DbgCreationInfo& ) = default;
	DbgCreationInfo& operator= ( const DbgCreationInfo& ) = default;
	DbgCreationInfo( DbgCreationInfo&& other ) {
		creationPoint = std::move( other.creationPoint );
		other.origination = Origination::movedout;
		origination = Origination::movedin;
	}
	DbgCreationInfo& operator = ( DbgCreationInfo&& other ) {
		creationPoint = std::move( other.creationPoint );
		other.origination = Origination::movedout;
		origination = Origination::movedin;
		return *this;
	}
	void init( Origination o ) { 
		creationPoint.init(); 
		origination = o;
	}
	void swap( DbgCreationInfo& other ) {
		auto tmpo = origination;
		origination = other.origination;
		other.origination = tmpo;
		nodecpp::StackInfo tmpc( std::move( creationPoint ) );
		creationPoint = std::move( other.creationPoint );
		other.creationPoint = std::move( tmpc );
	}
	std::string toStr() const {
		if ( nodecpp::impl::isDataStackInfo( creationPoint ) )
			return fmt::format( "\tObject has been created at {} here:\n{}\n", nodecpp::impl::whenTakenStackInfo( creationPoint ), nodecpp::impl::whereTakenStackInfo( creationPoint ).c_str() );
		else
			return "";
	}
};
struct DbgDestructionInfo
{
	enum Destruction { alive=0, dtoring=1, resetting=2, nulling=3 };
	Destruction destruction = Destruction::alive;
	static constexpr const char* destructionStr[] = { "", "destructing its owning_ptr<>", "resetting its own_ptr<>", "nulling its owning_ptr<>" };
	nodecpp::StackInfo destructionPoint;
	void init( Destruction d ) { 
		destructionPoint.init(); 
		destruction = d;
	}
	std::string toStr() const {
		if ( nodecpp::impl::isDataStackInfo( destructionPoint ) )
			return fmt::format( "\tObject has been deleted via {} at {} here:\n{}\n", destructionStr[destruction], nodecpp::impl::whenTakenStackInfo( destructionPoint ), nodecpp::impl::whereTakenStackInfo( destructionPoint ).c_str() );
		else
			return fmt::format( "\tObject has been deleted via {}\n", destructionStr[destruction] );
	}
};
struct DbgCreationAndDestructionInfo
{
	DbgCreationInfo creationInfo;
	DbgDestructionInfo destructionInfo;
	std::string toStr() const {
		return creationInfo.toStr() + destructionInfo.toStr();
	}
};
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO


template<class T> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_impl; // forward declaration
template<class T> class nullable_ptr_base_impl; // forward declaration
template<class T> class nullable_ptr_impl; // forward declaration
template<class T> class soft_this_ptr_impl; // forward declaration
namespace lib_helpers { template<class T> class soft_ptr_with_zero_offset_impl; } // forward declaration

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
#ifdef NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
	static_assert( sizeof(PtrWishFlagsForSoftPtrList) == 8 );
#endif

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
//			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, idx < (1<<19) ); // TODO
				//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "at 2nd block 0x{:x}: inserted idx {} with 0x{:x}", (size_t)this, idx, (size_t)ptr);
			return idx;
		}
		void resetPtr( size_t idx, void* newPtr ) {
				//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "at 2nd block 0x{:x}: about to reset idx {} to 0x{:x}", (size_t)this, idx, (size_t)newPtr);
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
				//otherAllockedSlots.setPtr( newOtherAllockedSlots );
				ret->addToFreeList( ret->slots + present->otherAllockedCnt, newSize - present->otherAllockedCnt );
				deallocate( present );
				ret->otherAllockedCnt = newSize;
				//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "after 2nd block relocation: ret = 0x{:x}, ret->otherAllockedCnt = {} (reallocation)", (size_t)ret, ret->otherAllockedCnt );
				return ret;
			}
			else {
				//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedCnt == 0 );
				SecondCBHeader* ret = reinterpret_cast<SecondCBHeader*>( allocate( (secondBlockStartSize + 2) * sizeof(PtrWishFlagsForSoftPtrList) ) );
				ret->otherAllockedCnt = secondBlockStartSize;
				//present->firstFree->set(nullptr);
				//otherAllockedSlots.setPtr( reinterpret_cast<PtrWishFlagsForSoftPtrList*>( allocate( otherAllockedCnt * sizeof(PtrWishFlagsForSoftPtrList) ) ) );
				ret->addToFreeList( ret->slots, secondBlockStartSize );
				//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "after 2nd block relocation: ret = 0x{:x}, ret->otherAllockedCnt = {} (ini allocation)", (size_t)ret, ret->otherAllockedCnt );
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
	template <class T>
	void dbgCheckValidity() const
	{
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( slots[i].isUsed() )
			{
				//bool ok = reinterpret_cast<soft_ptr_impl<T>*>(slots[i].getPtr())->getIdx_() == i;
				NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
					(reinterpret_cast<soft_ptr_impl<T>*>(slots[i].getPtr())->getIdx_() == i), 
					"getIdx_() = {}, i = {}", reinterpret_cast<soft_ptr_impl<T>*>(slots[i].getPtr())->getIdx_(), i );
			}
		if ( otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( otherAllockedSlots.getPtr()->slots[i].isUsed() )
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
						(reinterpret_cast<soft_ptr_impl<T>*>(otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_() == i + FirstControlBlock::maxSlots), 
						"getIdx_() = {}, expected: {}", reinterpret_cast<soft_ptr_impl<T>*>(otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_(), i + FirstControlBlock::maxSlots );
	}
#else
	template <class T>
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
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB initialized at 0x{:x}, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
		dbgCheckValidity<void>();
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
		dbgCheckValidity<void>();
	}
	void enlargeSecondBlock() {
		otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
	}
	size_t insert( void* ptr ) {
		dbgCheckValidity<void>();
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
					//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: inserted 0x{:x} at idx {}", (size_t)this, (size_t)ptr, i );
					return i;
				}
		}
		//else
		{
			if ( otherAllockedSlots.getPtr() == nullptr || otherAllockedSlots.getPtr()->firstFree == nullptr )
			{
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: about to reset 2nd block, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
				otherAllockedSlots.setPtr( SecondCBHeader::reallocate( otherAllockedSlots.getPtr() ) );
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: after reset 2nd block, otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
			}
			NODECPP_ASSERT( nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, otherAllockedSlots.getPtr() && otherAllockedSlots.getPtr()->firstFree );
			size_t idx = maxSlots + otherAllockedSlots.getPtr()->insert( ptr );
					//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: inserted 0x{:x} at idx {}", (size_t)this, (size_t)ptr, idx );
			return idx;
		}
	}
	void resetPtr( size_t idx, void* newPtr ) {
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: about to reset to 0x{:x} at idx {}", (size_t)this, (size_t)newPtr, idx );
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
		dbgCheckValidity<void>();
	}
	void remove( size_t idx ) {
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: about to remove at idx {}", (size_t)this, idx );
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
		dbgCheckValidity<void>();
	}
	void clear() {
		//nodecpp::log::default_log::error( nodecpp::log::ModuleID(nodecpp::safememory_module_id), "1CB 0x{:x}: clear(), otherAllockedSlots.getPtr() = 0x{:x}", (size_t)this, (size_t)(otherAllockedSlots.getPtr()) );
		if ( otherAllockedSlots.getPtr() != nullptr )
			otherAllockedSlots.getPtr()->dealloc();
		otherAllockedSlots.setZombie();
		//dbgCheckValidity<void>();
	}
	bool isZombie() { return otherAllockedSlots.isZombie(); }
};
//static_assert( sizeof(FirstControlBlock) == 32 );



inline
FirstControlBlock* getControlBlock_(const void* t) { return reinterpret_cast<FirstControlBlock*>(const_cast<void*>(t)) - 1; }
inline
uint8_t* getAllocatedBlock_(const void* t) { return reinterpret_cast<uint8_t*>(getControlBlock_(t)) + getPrefixByteCount(); }
inline
uint8_t* getAllocatedBlockFromControlBlock_(void* cb) { return reinterpret_cast<uint8_t*>(cb) + getPrefixByteCount(); }
inline
void* getPtrToAllocatedObjectFromControlBlock_( void* allocObjPtr ) { return (reinterpret_cast<FirstControlBlock*>(allocObjPtr)) + 1; }


//struct make_owning_t {};
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

	friend class lib_helpers::soft_ptr_with_zero_offset_impl<T>;
	template<class TT>
	friend class lib_helpers::soft_ptr_with_zero_offset_impl;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
	using base_pointer_t = nodecpp::platform::ptrwithdatastructsdefs::generic_ptr_with_zombie_property_; 
#else
	using base_pointer_t = nodecpp::platform::ptr_with_zombie_property; 
#endif // SAFE_PTR_DEBUG_MODE
	template<class TT>
	struct ObjectPointer : base_pointer_t {
	public:
		void setPtr( const void* ptr_ ) { base_pointer_t::init(ptr_); }
		void setTypedPtr( T* ptr_ ) { base_pointer_t::init(ptr_); }
		void* getPtr() const { return base_pointer_t::get_ptr(); }
		TT* getTypedPtr() const { return reinterpret_cast<TT*>( base_pointer_t::get_ptr() ); }
		void setZombie() { base_pointer_t::set_zombie(); }
		//bool isZombie() const { return base_pointer_t::is_sombie(); }
	};
	ObjectPointer<T> t; // the only data member!
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
	DbgCreationInfo creationInfo;
	void dbgSetDestructionPointInfo( DbgDestructionInfo::Destruction reason )
	{
		DbgDestructionInfo destructionInfo;
		destructionInfo.init( reason );
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
			{
				reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->dbgObjectStatus.destructionInfo = destructionInfo;
				reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->dbgObjectStatus.creationInfo = creationInfo;
			}
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
				{
					reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->dbgObjectStatus.destructionInfo = destructionInfo;
					reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->dbgObjectStatus.creationInfo = creationInfo;
				}
	}
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

	FirstControlBlock* getControlBlock() { return getControlBlock_(t.getPtr()); }
	const FirstControlBlock* getControlBlock() const { return getControlBlock_(t.getPtr()); }
	uint8_t* getAllocatedBlock() {return getAllocatedBlock_(t.getPtr()); }

	void updatePtrForListItemsWithInvalidPtr()
	{
		FirstControlBlock* cb = getControlBlock();
		for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
				reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->invalidatePtr();
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
					reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->invalidatePtr();
	}

#ifdef NODECPP_SAFEMEMORY_HEAVY_DEBUG
	void dbgCheckValidity() const
	{
		if ( t.getPtr() == nullptr )
			return;
		const FirstControlBlock* cb = getControlBlock();
		cb->dbgCheckValidity<T>();
		/*for ( size_t i=0; i<FirstControlBlock::maxSlots; ++i )
			if ( cb->slots[i].isUsed() )
			{
				//bool ok = reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->getIdx_() == i;
				NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
					(reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->getIdx_() == i), 
					"getIdx_() = {}, i = {}", reinterpret_cast<soft_ptr_impl<T>*>(cb->slots[i].getPtr())->getIdx_(), i );
			}
		if ( cb->otherAllockedSlots.getPtr() )
			for ( size_t i=0; i<cb->otherAllockedSlots.getPtr()->otherAllockedCnt; ++i )
				if ( cb->otherAllockedSlots.getPtr()->slots[i].isUsed() )
					NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 
						(reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_() == i + FirstControlBlock::maxSlots), 
						"getIdx_() = {}, expected: {}", reinterpret_cast<soft_ptr_impl<T>*>(cb->otherAllockedSlots.getPtr()->slots[i].getPtr())->getIdx_(), i + FirstControlBlock::maxSlots );*/
	}
#else
	void dbgCheckValidity() const {}
#endif // NODECPP_SAFEMEMORY_HEAVY_DEBUG

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	owning_ptr_base_impl( make_owning_t, T* t_ ) // make it private with a friend make_owning_impl()!
	{
		t.setPtr( t_ );
		getControlBlock()->init();
		dbgCheckValidity();
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo.init( DbgCreationInfo::Origination::created );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
	}
	owning_ptr_base_impl()
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo.init( DbgCreationInfo::Origination::inizero );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		t.setPtr( nullptr );
	}
	owning_ptr_base_impl( owning_ptr_base_impl<T>& other ) = delete;
	owning_ptr_base_impl& operator = ( owning_ptr_base_impl<T>& other ) = delete;
	owning_ptr_base_impl( owning_ptr_base_impl<T>&& other )
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo = std::move( other.creationInfo );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		t.setTypedPtr( other.t.getTypedPtr() );
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
	}
	owning_ptr_base_impl& operator = ( owning_ptr_base_impl<T>&& other )
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo = std::move( other.creationInfo );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		if ( this == &other ) return *this;
		t.setTypedPtr( other.t.getTypedPtr() );
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
		return *this;
	}
	template<class T1>
	owning_ptr_base_impl( owning_ptr_base_impl<T1>&& other )
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo = std::move( other.creationInfo );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		t.setTypedPtr( other.t.getTypedPtr() ); // implicit cast, if at all possible
		other.t.init( nullptr );
		other.dbgCheckValidity();
		dbgCheckValidity();
	}
	template<class T1>
	owning_ptr_base_impl& operator = ( owning_ptr_base_impl<T>&& other )
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo = std::move( other.creationInfo );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		if ( this == &other ) return *this;
		t = other.t; // implicit cast, if at all possible
		other.t = nullptr;
		other.dbgCheckValidity();
		dbgCheckValidity();
		return *this;
	}
	owning_ptr_base_impl( std::nullptr_t nulp )
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo.init( DbgCreationInfo::Origination::inizero );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		t.setPtr( nullptr );
	}
	owning_ptr_base_impl& operator = ( std::nullptr_t nulp )
	{
		reset();
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo.init( DbgCreationInfo::Origination::reset );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		return *this;
	}
	~owning_ptr_base_impl()
	{
		// NOTE: if this class is used (for lib-internal purposes), it is assumed that do_delete() is already called
		//       this is (so far) the only difference with owning_pt_impl where do_delete() is made private and is only called from ctor
	}

	void do_delete()
	{
		//dbgValidateList();
		if ( NODECPP_LIKELY(t.getTypedPtr()) )
		{
			destruct( t.getTypedPtr() );
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
			updatePtrForListItemsWithInvalidPtr();
			dbgSetDestructionPointInfo( DbgDestructionInfo::Destruction::dtoring );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
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
			destruct( t.getTypedPtr() );
			updatePtrForListItemsWithInvalidPtr();
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
			dbgSetDestructionPointInfo( DbgDestructionInfo::Destruction::resetting );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
			zombieDeallocate( getAllocatedBlock_(t.getTypedPtr()) );
			getControlBlock()->clear();
			t.setPtr( nullptr );
		}
		dbgCheckValidity();
	}

	void swap( owning_ptr_base_impl<T>& other )
	{
		T* tmp = t;
		t = other.t;
		other.t = tmp;
		other.dbgCheckValidity();
		dbgCheckValidity();
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		creationInfo.swap( other.creationInfo );
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
	}

	nullable_ptr_impl<T> get() const
	{
		nullable_ptr_impl<T> ret;
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

	bool operator == (const soft_ptr_impl<T>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const { return t.getTypedPtr() == other.getDereferencablePtr(); }

	bool operator != (const soft_ptr_impl<T>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const { return t.getTypedPtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return t.getTypedPtr() != nullptr; }

	// T* release() : prhibited by safity requirements

	explicit operator bool() const noexcept
	{
		return t.getTypedPtr() != nullptr;
	}
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

	friend class lib_helpers::soft_ptr_with_zero_offset_impl<T>;
	template<class TT>
	friend class lib_helpers::soft_ptr_with_zero_offset_impl;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

	using owning_ptr_base_impl<T>::do_delete;

public:
	static constexpr memory_safety is_safe = memory_safety::safe;

	owning_ptr_impl( make_owning_t mo, T* t_ ) : owning_ptr_base_impl<T>( mo, t_ ) {}
	owning_ptr_impl() : owning_ptr_base_impl<T>() {}
	owning_ptr_impl( owning_ptr_impl<T>& other ) = delete;
	owning_ptr_impl( owning_ptr_impl<T>&& other ) : owning_ptr_base_impl<T>( std::move(other) ) {}
	template<class T1>
	owning_ptr_impl( owning_ptr_impl<T1>&& other ) : owning_ptr_base_impl<T>( std::move(other) ) {}
	owning_ptr_impl( std::nullptr_t nulp ) : owning_ptr_base_impl<T>( nulp ) {}

	owning_ptr_impl& operator = ( owning_ptr_impl<T>& other ) = delete;
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

template<class _Ty,
	class... _Types,
	std::enable_if_t<!std::is_array<_Ty>::value, int> = 0>
	NODISCARD owning_ptr_impl<_Ty> make_owning_impl(_Types&&... _Args)
	{
	uint8_t* data = reinterpret_cast<uint8_t*>( zombieAllocate( sizeof(FirstControlBlock) - getPrefixByteCount() + sizeof(_Ty) ) );
	uint8_t* dataForObj = data + sizeof(FirstControlBlock) - getPrefixByteCount();
	owning_ptr_impl<_Ty> op(make_owning_t(), (_Ty*)(uintptr_t)(dataForObj));
	void* stackTmp = thg_stackPtrForMakeOwningCall;
	thg_stackPtrForMakeOwningCall = dataForObj;
	_Ty* objPtr = new ( dataForObj ) _Ty(::std::forward<_Types>(_Args)...);
	thg_stackPtrForMakeOwningCall = stackTmp;
	//return owning_ptr_impl<_Ty>(objPtr);
	return op;
	}


#define NODECPP_SAFE_PTR_USE_ON_STACK_OPTIMIZATION

#ifdef NODECPP_SAFE_PTR_USE_ON_STACK_OPTIMIZATION
//NODECPP_FORCEINLINE
//bool is_guaranteed_on_stack(void* p) { return nodecpp::platform::is_guaranteed_on_stack( p ); }
#define IF_IS_GUARANTEED_ON_STACK( ptr ) if ( nodecpp::platform::is_guaranteed_on_stack( (ptr) ) )
#define NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
#else
//constexpr bool is_guaranteed_on_stack(void*) { return false; }
#define IF_IS_GUARANTEED_ON_STACK( ptr ) if constexpr ( false )
#endif // NODECPP_SAFE_PTR_USE_ON_STACK_OPTIMIZATION


#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
extern thread_local size_t onStackSafePtrCreationCount; 
extern thread_local size_t onStackSafePtrDestructionCount;
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {++onStackSafePtrCreationCount;}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() { if ( isOnStack() ) {++onStackSafePtrDestructionCount;} }
#else
#define INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT() {}
#define INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT() {}
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

template<class T>
class soft_ptr_base_impl
{
	friend class owning_ptr_base_impl<T>;
	friend class owning_ptr_impl<T>;
	template<class TT>
	friend class soft_ptr_base_impl;
	template<class TT>
	friend class soft_ptr_impl;

	friend class lib_helpers::soft_ptr_with_zero_offset_impl<T>;
	template<class TT>
	friend class lib_helpers::soft_ptr_with_zero_offset_impl;

	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1> );
	friend struct FirstControlBlock;

	template<class TT>
	friend class soft_ptr_base_no_checks;
	template<class TT>
	friend class soft_ptr_no_checks;

#ifdef NODECPP_SAFE_PTR_DEBUG_MODE
#ifdef NODECPP_X64
	using PointersT = nodecpp::platform::ptrwithdatastructsdefs::generic_allocated_ptr_and_ptr_and_data_and_flags_<32,1>; 
#else
	using PointersT = nodecpp::platform::ptrwithdatastructsdefs::generic_allocated_ptr_and_ptr_and_data_and_flags_<26,1>; 
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
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
	DbgCreationAndDestructionInfo dbgObjectStatus;
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

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

	soft_ptr_base_impl(FirstControlBlock* cb, T* t) // to be used for only types annotaded as [[nodecpp::owning_only]]
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, cb != nullptr );
		IF_IS_GUARANTEED_ON_STACK( this )
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

	static constexpr memory_safety is_safe = memory_safety::safe;

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

	soft_ptr_base_impl() {}

	template<class T1>
	soft_ptr_base_impl( const owning_ptr_base_impl<T1>& owner )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl<T>& operator = ( const owning_ptr_base_impl<T1>& owner )
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
	soft_ptr_base_impl( const owning_ptr_impl<T1>& owner )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl<T>& operator = ( const owning_ptr_impl<T1>& owner )
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
	soft_ptr_base_impl( const soft_ptr_base_impl<T1>& other )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl<T>& operator = ( const soft_ptr_base_impl<T1>& other )
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
	soft_ptr_base_impl( const soft_ptr_base_impl<T>& other )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl<T>& operator = ( soft_ptr_base_impl<T>& other )
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


	soft_ptr_base_impl( soft_ptr_base_impl<T>&& other )
	{
		if ( this == &other ) return;
		bool otherOnStack = other.isOnStack();
		IF_IS_GUARANTEED_ON_STACK( this )
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
				pointers.copy_from( other.pointers );
				if ( other.getDereferencablePtr() )
					getControlBlock(getAllocatedPtr())->resetPtr(getIdx_(), this);
				other.init(PointersT::max_data);
			}
		}
		other.dbgCheckMySlotConsistency();
		dbgCheckMySlotConsistency();
	}

	soft_ptr_base_impl<T>& operator = ( soft_ptr_base_impl<T>&& other )
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
				pointers.copy_from( other.pointers );
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
	soft_ptr_base_impl( const owning_ptr_base_impl<T1>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl( const owning_ptr_impl<T1>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl( const soft_ptr_base_impl<T1>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
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
	soft_ptr_base_impl( const soft_ptr_base_impl<T>& other, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(other.getAllocatedPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
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

	soft_ptr_base_impl( std::nullptr_t nulp ) {}
	soft_ptr_base_impl& operator = ( std::nullptr_t nulp )
	{
		reset();
		return *this;
	}

	void swap( soft_ptr_base_impl<T>& other )
	{
		bool iWasOnStack = isOnStack();
		bool otherWasOnStack = other.isOnStack();
		pointers.swap( other.pointers );
		if ( iWasOnStack )
		{
			if ( otherWasOnStack )
			{
				NODECPP_ASSERT( nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, isOnStack() );
				NODECPP_ASSERT( nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, other.isOnStack() );
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

	nullable_ptr_impl<T> get() const
	{
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getDereferencablePtr() != nullptr );
		nullable_ptr_impl<T> ret;
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

	bool operator == (const owning_ptr_base_impl<T>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }
	bool operator == (const owning_ptr_impl<T>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const { return getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_base_impl<T>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1>
	bool operator == (const soft_ptr_base_impl<T1>& other ) const { return getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr_base_impl<T>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }
	bool operator != (const owning_ptr_impl<T>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const { return getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_base_impl<T>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1>
	bool operator != (const soft_ptr_base_impl<T1>& other ) const { return getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return getDereferencablePtr() != nullptr; }

	~soft_ptr_base_impl()
	{
		dbgCheckMySlotConsistency();
		INCREMENT_ONSTACK_SAFE_PTR_DESTRUCTION_COUNT()
		if( getDereferencablePtr() != nullptr ) {
			NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, getAllocatedPtr() );
			if ( getIdx_() != PointersT::max_data )
			{
				NODECPP_ASSERT( nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !isOnStack() );
				getControlBlock()->remove(getIdx_());
			}
			setPtrZombie();
			forcePreviousChangesToThisInDtor(this); // force compilers to apply the above instruction
		}
	}
};

template<class T>
soft_ptr_impl<T> soft_ptr_in_constructor_impl(T* ptr) {
	FirstControlBlock* cbPtr = nullptr;
	cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
	void* allocatedPtr = getAllocatedBlockFromControlBlock_( getAllocatedBlock_(cbPtr) );
	if ( allocatedPtr == nullptr )
		throwPointerOutOfRange();
	FirstControlBlock* cb = cbPtr;
	return soft_ptr_impl<T>( cb, ptr );
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

	friend class lib_helpers::soft_ptr_with_zero_offset_impl<T>;
	template<class TT>
	friend class lib_helpers::soft_ptr_with_zero_offset_impl;

	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_static_cast_impl( soft_ptr_impl<TT1> );
	template<class TT, class TT1>
	friend soft_ptr_impl<TT> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<TT1> );
	friend struct FirstControlBlock;

private:
	friend class soft_this_ptr_impl<T>;
	template<class TT>
	friend class soft_this_ptr_impl;
	template<class TT>
	friend soft_ptr_impl<TT> soft_ptr_in_constructor_impl(TT* ptr);
	friend soft_ptr_impl<T> soft_ptr_in_constructor_impl<>(T* ptr);
	soft_ptr_impl(FirstControlBlock* cb, T* t) : soft_ptr_base_impl<T>(cb, t) {} // to be used for only types annotaded as [[nodecpp::owning_only]]

public:
	soft_ptr_impl() : soft_ptr_base_impl<T>()
	{
		this->init( soft_ptr_base_impl<T>::PointersT::max_data );
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}


	template<class T1>
	soft_ptr_impl( const owning_ptr_base_impl<T1>& owner ) : soft_ptr_base_impl<T>(owner) {}
	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1>& owner ) : soft_ptr_base_impl<T>(owner) {}

	soft_ptr_impl( const owning_ptr_impl<T>& owner )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), this->getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}
	soft_ptr_impl( const owning_ptr_base_impl<T>& owner )
	{
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->initOnStack( owner.t.getTypedPtr(), owner.t.getTypedPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), this->getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}

	template<class T1>
	soft_ptr_impl<T>& operator = ( const owning_ptr_base_impl<T1>& owner )
	{
		soft_ptr_base_impl<T>::operator = (owner);
		return *this;
	}
	template<class T1>
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T1>& owner )
	{
		soft_ptr_base_impl<T>::operator = (owner);
		return *this;
	}

	soft_ptr_impl<T>& operator = ( const owning_ptr_base_impl<T>& owner )
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
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
		return *this;
	}
	soft_ptr_impl<T>& operator = ( const owning_ptr_impl<T>& owner )
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
				this->init( owner.t.getTypedPtr(), owner.t.getTypedPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
		return *this;
	}


	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1>& other ) : soft_ptr_base_impl<T>(other) {}
	template<class T1>
	soft_ptr_impl<T>& operator = ( const soft_ptr_impl<T1>& other )
	{
		soft_ptr_base_impl<T>::operator = (other);
		return *this;
	}
	soft_ptr_impl( const soft_ptr_impl<T>& other ) : soft_ptr_base_impl<T>(other) {}
	soft_ptr_impl<T>& operator = ( soft_ptr_impl<T>& other )
	{
		soft_ptr_base_impl<T>::operator = (other);
		return *this;
	}


	soft_ptr_impl( soft_ptr_impl<T>&& other ) : soft_ptr_base_impl<T>( std::move(other) ) {}

	soft_ptr_impl<T>& operator = ( soft_ptr_impl<T>&& other )
	{
		soft_ptr_base_impl<T>::operator = ( std::move(other) );
		return *this;
	}

	template<class T1>
	soft_ptr_impl( const owning_ptr_base_impl<T1>& owner, T* t_ ) : soft_ptr_base_impl<T>(owner, t_) {}
	template<class T1>
	soft_ptr_impl( const owning_ptr_impl<T1>& owner, T* t_ ) : soft_ptr_base_impl<T>(owner, t_) {}

	soft_ptr_impl( const owning_ptr_base_impl<T>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			initOnStack( t_, owner.t.getPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				init( t_, owner.t.getPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t.getPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}
	soft_ptr_impl( const owning_ptr_impl<T>& owner, T* t_ )
	{
		if ( !isZombieablePointerInBlock( getAllocatedBlock_(owner.t.getPtr()), t_ ) )
			throwPointerOutOfRange();
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			initOnStack( t_, owner.t.getPtr() ); // automatic type conversion (if at all possible)
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		else
			if ( owner.t .getPtr())
				init( t_, owner.t.getPtr(), getControlBlock(owner.t.getPtr())->insert(this) ); // automatic type conversion (if at all possible)
			else
				init( t_, owner.t.getPtr(), soft_ptr_base_impl<T>::PointersT::max_data ); // automatic type conversion (if at all possible)
		this->dbgCheckMySlotConsistency();
	}

	template<class T1>
	soft_ptr_impl( const soft_ptr_impl<T1>& other, T* t_ ) : soft_ptr_base_impl<T>(other, t_) {}
	soft_ptr_impl( const soft_ptr_impl<T>& other, T* t_ ) : soft_ptr_base_impl<T>(other, t_) {}

	soft_ptr_impl( std::nullptr_t nulp ) : soft_ptr_base_impl<T>( nulp )
	{
		this->init( soft_ptr_base_impl<T>::PointersT::max_data );
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}
	soft_ptr_impl& operator = ( std::nullptr_t nulp )
	{
		soft_ptr_base_impl<T>::operator = (nulp);
		return *this;
	}

	void swap( soft_ptr_impl<T>& other )
	{
		soft_ptr_base_impl<T>::swap(other);
	}

#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
	void dbgTestForNullAndThrowNullPtrAccess() const
	{
		if ( this->getDereferencablePtr() == nullptr )
		{
			nodecpp::error::string_ref extra( this->dbgObjectStatus.toStr().c_str() );
			throw nodecpp::error::nodecpp_error(nodecpp::error::NODECPP_EXCEPTION::null_ptr_access, std::move( extra ) );
		}
	}
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

	nullable_ptr_impl<T> get() const
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		dbgTestForNullAndThrowNullPtrAccess();
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

		return soft_ptr_base_impl<T>::get();
	}

	T& operator * () const
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		dbgTestForNullAndThrowNullPtrAccess();
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

		checkNotNullAllSizes( this->getDereferencablePtr() );
		return *(this->getDereferencablePtr());
	}

	T* operator -> () const 
	{
#ifdef NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO
		dbgTestForNullAndThrowNullPtrAccess();
#endif // NODECPP_MEMORY_SAFETY_DBG_ADD_DESTRUCTION_INFO

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
		soft_ptr_base_impl<T>::reset();
	}

	bool operator == (const owning_ptr_base_impl<T>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	bool operator == (const owning_ptr_impl<T>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<T>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	bool operator != (const owning_ptr_base_impl<T>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	bool operator != (const owning_ptr_impl<T>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<T>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }
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
	soft_ptr_impl() : soft_ptr_base_impl<void>()
	{
		pointers.init( soft_ptr_base_impl<void>::PointersT::max_data );
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}


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

	soft_ptr_impl( std::nullptr_t nulp ) : soft_ptr_base_impl<void>( nulp )
	{
		pointers.init( soft_ptr_base_impl<void>::PointersT::max_data );
		IF_IS_GUARANTEED_ON_STACK( this )
		{
			this->setOnStack();
			INCREMENT_ONSTACK_SAFE_PTR_CREATION_COUNT()
		}
		this->dbgCheckMySlotConsistency();
	}
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
		soft_ptr_base_impl<void>::operator = ( std::move(other) );
		return *this;
	}

	void swap( soft_ptr_impl<void>& other )
	{
		soft_ptr_base_impl<void>::swap( other );
	}

	explicit operator bool() const noexcept
	{
		return this->getDereferencablePtr() != nullptr;
	}

	template<class T1> 
	bool operator == (const owning_ptr_base_impl<T1>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }
	template<class T1> 
	bool operator == (const owning_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() == other.t.getTypedPtr(); }

	bool operator == (const soft_ptr_impl<void>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }
	template<class T1>
	bool operator == (const soft_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() == other.getDereferencablePtr(); }

	template<class T1> 
	bool operator != (const owning_ptr_base_impl<T1>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }
	template<class T1> 
	bool operator != (const owning_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() != other.t.getTypedPtr(); }

	bool operator != (const soft_ptr_impl<void>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }
	template<class T1>
	bool operator != (const soft_ptr_impl<T1>& other ) const { return this->getDereferencablePtr() != other.getDereferencablePtr(); }

	bool operator == (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() == nullptr; }
	bool operator != (std::nullptr_t nullp ) const { NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::pedantic, nullp == nullptr); return this->getDereferencablePtr() != nullptr; }

	void reset()
	{
		soft_ptr_base_impl<void>::reset();
	}
};


template<class T, class T1>
soft_ptr_impl<T> soft_ptr_static_cast_impl( soft_ptr_impl<T1> p ) {
	soft_ptr_impl<T> ret(p,static_cast<T*>(p.getDereferencablePtr()));
	return ret;
}

template<class T, class T1>
soft_ptr_impl<T> soft_ptr_reinterpret_cast_impl( soft_ptr_impl<T1> p ) {
	soft_ptr_impl<T> ret(p,reinterpret_cast<T*>(p.getDereferencablePtr()));
	return ret;
}


template<class T>
class soft_this_ptr_impl
{
	FirstControlBlock* cbPtr = nullptr;
	uint32_t offset;

public:

	static constexpr memory_safety is_safe = memory_safety::safe;

	soft_this_ptr_impl()
	{
		cbPtr = getControlBlock_(thg_stackPtrForMakeOwningCall);
		uintptr_t delta = reinterpret_cast<uint8_t*>(this) - reinterpret_cast<uint8_t*>(cbPtr);
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, delta <= UINT32_MAX, "delta = 0x{:x}", delta );
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
		//return soft_ptr_impl<T>( allocatedPtr, ptr );
		//FirstControlBlock* cb = cbPtr;
		FirstControlBlock* cb = reinterpret_cast<FirstControlBlock*>( reinterpret_cast<uint8_t*>(this) - offset );
		return soft_ptr_impl<TT>( cb, ptr );
	}

	~soft_this_ptr_impl()
	{
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

} // namespace nodecpp::safememory

#endif // SAFE_PTR_IMPL_H
