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

#ifndef SAFE_PTR_COMMON_H
#define SAFE_PTR_COMMON_H

#include <foundation.h>
#include <nodecpp_assert.h>
#include <log.h>
#include <memory>
#include <stdint.h>
#include <safememory/detail/checker_attributes.h>
#include <allocator_template.h>
#include "memory_safety.h"

#if NODECPP_MEMORY_SAFETY == 0
#define NODECPP_MEMORY_SAFETY_ON_DEMAND
#endif

#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif


#if defined NODECPP_X64 && !defined NODECPP_NOT_USING_IIBMALLOC
#define NODECPP_USE_IIBMALLOC
#else
#define NODECPP_USE_NEW_DELETE_ALLOC
#endif


namespace safememory::detail {
enum class StdAllocEnforcer { enforce };
} // namespace safememory::detail


#ifdef NODECPP_USE_IIBMALLOC

#include <iibmalloc.h>
// using namespace nodecpp::iibmalloc;
namespace safememory::detail
{
using nodecpp::iibmalloc::g_CurrentAllocManager;
using nodecpp::iibmalloc::guaranteed_prefix_size;
using nodecpp::iibmalloc::ALIGNMENT;

#ifndef NODECPP_MEMORY_SAFETY_ON_DEMAND

NODECPP_FORCEINLINE void* allocate( size_t sz, size_t alignment ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->allocate( sz ); } // TODO: proper implementation for alignment
NODECPP_FORCEINLINE void* allocate( size_t sz ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->allocate( sz ); }
template<size_t alignment>
NODECPP_FORCEINLINE void* allocateAligned( size_t sz ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->allocateAligned<alignment>( sz ); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); g_CurrentAllocManager->deallocate( ptr ); }
NODECPP_FORCEINLINE void deallocate( void* ptr, size_t alignment ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); g_CurrentAllocManager->deallocate( ptr ); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->zombieableAllocate( sz ); }
template<size_t sz, size_t alignment>
NODECPP_FORCEINLINE void* zombieAllocateAligned() { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->zombieableAllocateAligned<sz, alignment>(); }
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); g_CurrentAllocManager->zombieableDeallocate( ptr ); }
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->isZombieablePointerInBlock( allocatedPtr, ptr ); }
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE bool isPointerNotZombie(void* ptr ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->isPointerNotZombie( ptr ); }
inline bool doZombieEarlyDetection( bool doIt = true ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); return g_CurrentAllocManager->doZombieEarlyDetection( doIt ); }
#else
constexpr bool isPointerNotZombie(void* ptr ) { return true; }
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { static_assert(guaranteed_prefix_size <= 3*sizeof(void*)); return guaranteed_prefix_size; }
inline void killAllZombies() { g_CurrentAllocManager->killAllZombies(); }

#else // NODECPP_MEMORY_SAFETY_ON_DEMAND

NODECPP_FORCEINLINE void* allocate( size_t sz, size_t alignment )  // TODO: proper implementation for alignment
{ 
	if ( g_CurrentAllocManager == nullptr )
	{
		void* ret = ::operator new [] (sz, std::align_val_t(alignment)); 
		return ret;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->allocate( sz );

}

NODECPP_FORCEINLINE void* allocate( size_t sz )
{
	if ( g_CurrentAllocManager == nullptr )
	{
		void* ret = ::operator new [] (sz); 
		return ret;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->allocate( sz );
}

template<size_t alignment>
NODECPP_FORCEINLINE void* allocateAligned( size_t sz )
{
	if ( g_CurrentAllocManager == nullptr )
		return allocate( sz, alignment );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->allocateAligned<alignment>( sz );
}

NODECPP_FORCEINLINE void deallocate( void* ptr, uint16_t allocatorID )
{
	if ( allocatorID == 0 )
	{
		auto* formerAlloc = ::nodecpp::iibmalloc::setCurrneAllocator( nullptr );
		::operator delete [] (ptr);
		::nodecpp::iibmalloc::setCurrneAllocator( formerAlloc );
		return;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager->allocatorID() == allocatorID, "{} vs. {}", g_CurrentAllocManager->allocatorID(), allocatorID ); 
	g_CurrentAllocManager->deallocate( ptr );
}

NODECPP_FORCEINLINE void deallocate( void* ptr, bool isStdHeap )
{
	if ( isStdHeap )
	{
		auto* formerAlloc = ::nodecpp::iibmalloc::setCurrneAllocator( nullptr );
		::operator delete [] (ptr);
		::nodecpp::iibmalloc::setCurrneAllocator( formerAlloc );
		return;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	g_CurrentAllocManager->deallocate( ptr );
}

NODECPP_FORCEINLINE void deallocate( void* ptr, size_t alignment, uint16_t allocatorID )
{
	if ( allocatorID == 0 )
	{
		auto* formerAlloc = ::nodecpp::iibmalloc::setCurrneAllocator( nullptr );
		::operator delete [] (ptr, std::align_val_t(alignment));
		::nodecpp::iibmalloc::setCurrneAllocator( formerAlloc );
		return;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager->allocatorID() == allocatorID, "{} vs. {}", g_CurrentAllocManager->allocatorID(), allocatorID ); 
	g_CurrentAllocManager->deallocate( ptr );
}

NODECPP_FORCEINLINE void* zombieAllocate( size_t sz )
{
	if ( g_CurrentAllocManager == nullptr )
		return allocate( sz );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->zombieableAllocate( sz );
}

template<size_t sz, size_t alignment>
NODECPP_FORCEINLINE void* zombieAllocateAligned()
{
	if ( g_CurrentAllocManager == nullptr )
		return allocate( sz, alignment );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->zombieableAllocateAligned<sz, alignment>();
}

NODECPP_FORCEINLINE void zombieDeallocate( void* ptr, uint16_t allocatorID )
{
	if ( allocatorID == 0 )
	{
		auto* formerAlloc = ::nodecpp::iibmalloc::setCurrneAllocator( nullptr );
		::operator delete [] (ptr);
		::nodecpp::iibmalloc::setCurrneAllocator( formerAlloc );
		return;
	}
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager->allocatorID() == allocatorID, "{} vs. {}", g_CurrentAllocManager->allocatorID(), allocatorID ); 
	g_CurrentAllocManager->zombieableDeallocate( ptr );
}

NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr )
{
	if ( g_CurrentAllocManager == nullptr )
		return true;
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->isZombieablePointerInBlock( allocatedPtr, ptr );
}

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE bool isPointerNotZombie(void* ptr )
{
	if ( g_CurrentAllocManager == nullptr )
		return true;
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->isPointerNotZombie( ptr );
}

inline bool doZombieEarlyDetection( bool doIt = true )
{
	if ( g_CurrentAllocManager == nullptr )
		return false;
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	return g_CurrentAllocManager->doZombieEarlyDetection( doIt );
}
#else
constexpr bool isPointerNotZombie(void* ptr ) { return true; }
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION

NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount()
{
	static_assert(guaranteed_prefix_size <= 3*sizeof(void*)); 
	return guaranteed_prefix_size;
}

inline void killAllZombies()
{
	if ( g_CurrentAllocManager == nullptr )
		return;
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr ); 
	g_CurrentAllocManager->killAllZombies();
}

#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND

NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return ALIGNMENT; }

struct IIBRawAllocator
{
	static constexpr size_t guaranteed_alignment = NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT;
	template<size_t alignment = 0> 
	static NODECPP_FORCEINLINE void* allocate( size_t allocSize ) { return ::safememory::detail::allocateAligned<alignment>( allocSize ); }
//	static NODECPP_FORCEINLINE void* allocate( size_t allocSize, size_t allignment ) { return ::safememory::detail::allocate( allocSize ); }
	template<size_t alignment = 0> 
	static NODECPP_FORCEINLINE void deallocate( void* ptr ) { return ::safememory::detail::deallocate( ptr ); }
};

template<class _Ty>
using iiballocator = nodecpp::selective_allocator<IIBRawAllocator, _Ty>;
template< class T1, class T2 >
bool operator==( const iiballocator<T1>& lhs, const iiballocator<T2>& rhs ) noexcept { return true; }
template< class T1, class T2 >
bool operator!=( const iiballocator<T1>& lhs, const iiballocator<T2>& rhs ) noexcept { return false; }

} // namespace safememory::detail

#elif defined NODECPP_USE_NEW_DELETE_ALLOC

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
#include <map>
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
namespace safememory::detail
{
template<class T>
using iiballocator =  std::allocator<T>;

// NOTE: while being non-optimal, following calls provide safety guarantees and can be used at least for debug purposes
extern thread_local void** zombieList_; // must be set to zero at the beginning of a thread function
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
extern thread_local std::map<uint8_t*, size_t, std::greater<uint8_t*>> zombieMap;
extern thread_local bool doZombieEarlyDetection_;
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION

inline void killAllZombies()
{
	while ( zombieList_ != nullptr )
	{
		void** next = reinterpret_cast<void**>( *zombieList_ );
		delete [] zombieList_;
		zombieList_ = next;
	}
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, doZombieEarlyDetection_ || ( !doZombieEarlyDetection_ && zombieMap.empty() ) );
	zombieMap.clear();
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
}
NODECPP_FORCEINLINE void* allocate( size_t sz, size_t alignment ) { void* ret = ::operator new [] (sz, std::align_val_t(alignment)); return ret; } // TODO: proper implementation for alignment
NODECPP_FORCEINLINE void* allocate( size_t sz ) { void* ret = ::operator new [] (sz); return ret; }
template<size_t alignment>
NODECPP_FORCEINLINE void* allocateAligned( size_t sz ) { return allocate( sz, alignment ); }
NODECPP_FORCEINLINE void deallocate( void* ptr, size_t alignment ) { ::operator delete [] (ptr, std::align_val_t(alignment)); }
NODECPP_FORCEINLINE void deallocate( void* ptr ) { ::operator delete [] (ptr); }
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz ) { 
	uint8_t* ret = new uint8_t[ 4 * sizeof(uint64_t) + sz ]; 
	*reinterpret_cast<uint64_t*>(ret) = sz; 
	return ret + 4 * sizeof(uint64_t);
}
NODECPP_FORCEINLINE void* zombieAllocate( size_t sz, size_t alignment ) { 
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, alignment <= 4 * sizeof(uint64_t), "alignment = {}", alignment );
	return zombieAllocate( sz );
}
template<size_t sz, size_t alignment>
NODECPP_FORCEINLINE void* zombieAllocateAligned() {
	return zombieAllocate(sz, alignment);
}
NODECPP_FORCEINLINE void zombieDeallocate( void* ptr ) { 
	void** blockStart = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ptr) - 4 * sizeof(uint64_t)); 
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	size_t allocSize = *reinterpret_cast<uint64_t*>(blockStart);
	zombieMap.insert( std::make_pair( reinterpret_cast<uint8_t*>(blockStart), 4 * sizeof(uint64_t) + allocSize ) );
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	*blockStart = zombieList_; 
	zombieList_ = blockStart;
}
NODECPP_FORCEINLINE bool isZombieablePointerInBlock(void* allocatedPtr, void* ptr ) { return ptr >= allocatedPtr && reinterpret_cast<uint8_t*>(allocatedPtr) + *(reinterpret_cast<uint64_t*>(allocatedPtr) - 2) > reinterpret_cast<uint8_t*>(ptr); }
#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE bool isPointerNotZombie(const void* ptr ) { 
	auto iter = zombieMap.lower_bound( reinterpret_cast<uint8_t*>( const_cast<void*>(ptr) ) );
	if ( iter != zombieMap.end() )
		return reinterpret_cast<uint8_t*>( const_cast<void*>(ptr) ) >= iter->first + iter->second;
	else
		return true;
}
inline bool doZombieEarlyDetection( bool doIt = true )
{
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, zombieMap.empty(), "to (re)set doZombieEarlyDetection() zombieMap must be empty" );
	bool ret = doZombieEarlyDetection_;
	doZombieEarlyDetection_ = doIt;
	return ret;
}
#else
constexpr bool isPointerNotZombie(const void* ptr ) { return true; }
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
NODECPP_FORCEINLINE constexpr size_t getPrefixByteCount() { return sizeof(uint64_t); }
NODECPP_FORCEINLINE size_t allocatorAlignmentSize() { return sizeof(void*); }
} //namespace safememory::detail

#else
#error at least some specific allocation functionality must be selected
#endif


namespace safememory::detail
{
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

#if NODECPP_MEMORY_SAFETY == 0
struct make_owning_t {uint16_t allocatorID; make_owning_t( int id ) {allocatorID = id;} };
#else
struct make_owning_t {};
#endif
template<class T> class owning_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration


//#define NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED
#ifdef NODECPP_DEBUG_COUNT_SOFT_PTR_ENABLED
extern thread_local std::size_t CountSoftPtrZeroOffsetDtor;
extern thread_local std::size_t CountSoftPtrBaseDtor;
#define NODECPP_DEBUG_COUNT_SOFT_PTR_BASE_DTOR() { ::safememory::detail::CountSoftPtrBaseDtor++; }
#define NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR() { ::safememory::detail::CountSoftPtrZeroOffsetDtor++; }
#else
#define NODECPP_DEBUG_COUNT_SOFT_PTR_BASE_DTOR() { }
#define NODECPP_DEBUG_COUNT_SOFT_PTR_ZERO_OFFSET_DTOR() { }
#endif

} // namespace safememory::detail


#endif // SAFE_PTR_COMMON_H
