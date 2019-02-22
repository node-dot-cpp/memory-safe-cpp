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
#include <memory>
#include <stdint.h>


namespace nodecpp::safememory
{
	constexpr uint64_t module_id = 2;
} // namespace nodecpp::safememory

#define NODECPP_USE_IIBMALLOC
//#define NODECPP_USE_NEW_DELETE_ALLOC

#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
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


namespace nodecpp::safememory
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

struct make_owning_t {};

enum class memory_safety { none, safe };

template<class T>
struct safeness_declarator {
	static constexpr memory_safety is_safe = memory_safety::safe; // by default
};

/* Sample of user-defined exclusion:
template<> struct nodecpp::safememory::safeness_declarator<double> { static constexpr bool is_safe = false; };
*/

// sample code (to be removed)
template<> struct nodecpp::safememory::safeness_declarator<double> { static constexpr memory_safety is_safe = memory_safety::none; };
namespace testing::dummy_objects {
struct StructureWithSoftPtrDeclaredUnsafe; // forward declaration
}
template<> struct nodecpp::safememory::safeness_declarator<nodecpp::safememory::testing::dummy_objects::StructureWithSoftPtrDeclaredUnsafe> { static constexpr memory_safety is_safe = memory_safety::none; }; // user-defined exclusion
// end of sample code (to be removed)

template<class T> class owning_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_impl; // forward declaration
template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration




} // namespace nodecpp::safememory

#endif // SAFE_PTR_COMMON_H
