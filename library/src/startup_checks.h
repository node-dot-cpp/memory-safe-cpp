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

#ifndef STARTUP_CHECKS_H
#define STARTUP_CHECKS_H

#include <foundation.h>
#include "safe_ptr.h"

namespace safememory::testing {

using namespace ::safememory::detail;

// For various testing purposes we will need misc objects (classes, ...) doing nothing but representing certain constructions to be tested
// Such objects are gathered in the namespace safememory::testing::dummy_objects
namespace dummy_objects {

	class PRNG
	{
		uint64_t seedVal;
	public:
		PRNG() { seedVal = 1; }
		PRNG( size_t seed_ ) { seedVal = seed_ ? seed_ : 1; }
		void seed( size_t seed_ ) { seedVal = seed_ ? seed_ : 1; }
		// based on implementation of xorshift by Arvid Gerstmann; see, for instance, https://arvid.io/2018/07/02/better-cxx-prng/
		NODECPP_FORCEINLINE uint32_t rng32() { uint64_t ret = seedVal * 0xd989bcacc137dcd5ull; seedVal ^= seedVal >> 11; seedVal ^= seedVal << 31; seedVal ^= seedVal >> 18; return uint32_t(ret >> 32ull); }
		NODECPP_FORCEINLINE uint64_t rng64() { uint64_t ret = rng32(); ret <<= 32; return ret + rng32(); }
		NODECPP_FORCEINLINE uint64_t rng64NoNull() { uint64_t ret; do { ret = rng32(); ret <<= 32; ret += rng32(); } while (ret == 0); return ret; }
#ifdef NODECPP_X64
		NODECPP_FORCEINLINE size_t rng() { return rng64(); }
		NODECPP_FORCEINLINE size_t rngNoNull() { return rng64NoNull(); }
#elif ( defined NODECPP_X86 )
		NODECPP_FORCEINLINE size_t rng() { return rng32(); }
		NODECPP_FORCEINLINE size_t rngNoNull() { uint32_t ret; do { ret = rng32(); } while (ret == 0); return ret; }
#else
		// well, you will have no problems, if you do not use the above calls at all...
		NODECPP_FORCEINLINE size_t rng() { return (size_t)(rng64()); }
		NODECPP_FORCEINLINE size_t rngNoNull() { return (size_t)(rng64NoNull()); }
#endif
	};

	class SmallBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthSmall() { return 0x1;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rngNoNull();}
		virtual ~SmallBase() { sn = 0;forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng();}
	};
	class SmallDerived : public SmallBase { 
	public: 
		uint64_t sn1; 
		int doSmthSmall() override {return 0x2;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rngNoNull(); SmallBase::init(rng.rng());} 
		virtual ~SmallDerived() {sn1 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng() && SmallBase::check(rng.rng());}
	};
	class Small : public SmallDerived { 
	public: 
		uint64_t sn2; 
		int doSmthSmall() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rngNoNull(); 
		SmallDerived::init(rng.rng());} 
		virtual ~Small() {sn2 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng() && SmallDerived::check(rng.rng());}
	};

	static constexpr size_t SZ = 0x200;
	
	class LargeBase { 
	public: 
		uint64_t ln[SZ]; 
		virtual int doSmthLarge() { return 0x100;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rngNoNull();}
		virtual ~LargeBase() { ln[0] = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = true; for ( size_t i=0; i<SZ;++i) ret = ret && ln[i] == rng.rng(); return ret;}
	};
	class LargeDerived : public LargeBase { 
	public: 
		uint64_t ln1[SZ]; 
		int doSmthLarge() override { return 0x200;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rngNoNull(); LargeBase::init(rng.rng());} 
		virtual ~LargeDerived() {ln1[0] = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = ln1[0] == rng.rng(); for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng(); return ret && LargeBase::check(rng.rng());}
	};
	class Large : public LargeDerived { 
	public: 
		uint64_t ln2; 
		int doSmthLarge() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); ln2 = rng.rngNoNull(); LargeDerived::init(rng.rng());} 
		virtual ~Large() {ln2 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return ln2 == rng.rng() && LargeDerived::check(rng.rng());}
	};

	class SomeBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthSome() { return 0x1;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rngNoNull();}
		virtual ~SomeBase() {}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng();}
	};
	class SomeDerived : public SomeBase { 
	public: 
		uint64_t sn1; 
		int doSmthSome() override {return 0x2;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rngNoNull(); SomeBase::init(rng.rng());} 
		virtual ~SomeDerived() {}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng() && SomeBase::check(rng.rng());}
	};
	class Some : public SomeDerived { 
	public: 
		uint64_t sn2; 
		int doSmthSome() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rngNoNull(); 
		SomeDerived::init(rng.rng());} 
		virtual ~Some() {}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng() && SomeDerived::check(rng.rng());}
	};

	struct JustDummyStruct {
		uint32_t n1;
		uint32_t n2;
	};

	class SomeWithSafePointers : public Some { 
	public: 
		uint64_t sn3; 
		soft_ptr<JustDummyStruct> spDummy;
		owning_ptr<JustDummyStruct> opDummy;
		SomeWithSafePointers( owning_ptr<JustDummyStruct>& somePtr ) { 
			opDummy = make_owning<JustDummyStruct>();
			spDummy = somePtr;
		}
		int doSmthSome() override { return 0x4; } 
		void init(size_t seed) {PRNG rng(seed); sn3 = rng.rngNoNull(); SomeDerived::init(rng.rng());} 
		virtual ~SomeWithSafePointers() {}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng() && SomeDerived::check(rng.rng());}
		struct MyNonPointerMembers
		{
			uint64_t sn; 
			uint64_t sn1; 
			uint64_t sn2; 
			uint64_t sn3; 
			bool operator == ( const MyNonPointerMembers& other ) const { return sn == other.sn && sn1 == other.sn1 && sn2 == other.sn2 && sn3 == other.sn3; }
		};
		MyNonPointerMembers getMyNonPointerMembers() {
			MyNonPointerMembers ret;
			ret.sn = sn;
			ret.sn1 = sn1;
			ret.sn2 = sn2;
			ret.sn3 = sn3;
			return ret;
		}
		struct AllMyDataStruct // added to ensure layout understanding
		{
			void* ptr;
			MyNonPointerMembers mm;
			soft_ptr<JustDummyStruct> spDummy;
			owning_ptr<JustDummyStruct> opDummy;
		};
	};
	static_assert( sizeof(SomeWithSafePointers::AllMyDataStruct) == sizeof(SomeWithSafePointers) );


} // namespace dummy_objects

class StartupChecker
{
	enum ChangeStatus { no = 0, yes = 1, potential = 2 };

	static void setAddressesOfChanges( dummy_objects::Small* obj, uint64_t** addr1, uint64_t** addr2, uint64_t** addr3 )
	{
		*addr1 = &(obj->sn);
		*addr2 = &(obj->sn1);
		*addr3 = &(obj->sn2);
	}

	static void setAddressesOfChanges( dummy_objects::Large* obj, uint64_t** addr1, uint64_t** addr2, uint64_t** addr3 )
	{
		*addr1 = &(obj->ln[0]);
		*addr2 = &(obj->ln1[0]);
		*addr3 = &(obj->ln2);
	}

	template<class T>
	static void checkBasicsT()
	{
		size_t rngSeed = 155;
		dummy_objects::PRNG rng(rngSeed);
		uint64_t rngCheckVal;

		rngCheckVal = rng.rng();

#ifdef NODECPP_USE_IIBMALLOC
#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
		if ( g_CurrentAllocManager != nullptr )
#else
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr );
#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND
		{
			uint8_t* mem4T = reinterpret_cast<uint8_t*>(g_CurrentAllocManager->allocate(sizeof(T)));
			uint8_t* mem4TCopy = reinterpret_cast<uint8_t*>(g_CurrentAllocManager->allocate(sizeof(T)));
			uint8_t* changeMap = reinterpret_cast<uint8_t*>(g_CurrentAllocManager->allocate(sizeof(T)));
			T* TPtr = new ( mem4T ) T;
			TPtr->init(rngCheckVal);
			NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, TPtr->check(rngCheckVal));
			memcpy( mem4TCopy, mem4T, sizeof(T) );

			// collect actual addresses of changes in destructors
			uint64_t* addr1;
			uint64_t* addr2;
			uint64_t* addr3;
			setAddressesOfChanges( TPtr, &addr1, &addr2, &addr3 );
			auto vmtVal = nodecpp::platform::backup_vmt_pointer( TPtr );

			memset( changeMap, ChangeStatus::no, sizeof(T) );
			memset( changeMap + ( ((uint8_t*)addr1) - mem4T ), ChangeStatus::yes, sizeof( uint64_t) );
			memset( changeMap + ( ((uint8_t*)addr2) - mem4T ), ChangeStatus::yes, sizeof( uint64_t) );
			memset( changeMap + ( ((uint8_t*)addr3) - mem4T ), ChangeStatus::yes, sizeof( uint64_t) );
			auto vmtPos = nodecpp::platform::get_vmt_pointer_size_pos();
			memset( changeMap + vmtPos.first, ChangeStatus::yes, vmtPos.second );

			destruct( TPtr );

			if( memcmp( mem4TCopy, mem4T, sizeof(T) ) != 0 )
			{
				// check explicitly that changes done in dtor actually happened as intended (fast detection)
				NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, *addr1 == 0 );
				NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, *addr2 == 0 );
				NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, *addr3 == 0 );

				for ( size_t i=0; i<sizeof(T); ++i )
					if ( mem4T[i] != mem4TCopy[i] ) 
						NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, changeMap[i] != ChangeStatus::no );
			}
			else
				NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, false ); // our intention in dtor was to change memory state under the object

			g_CurrentAllocManager->deallocate( mem4T );
			g_CurrentAllocManager->deallocate( mem4TCopy );
			g_CurrentAllocManager->deallocate( changeMap );
		}
#else
//#error not implemented (but implementation for any other allocator (or generalization) should not become a greate task anyway)
#endif
	}

	static void checkSafePointers_()
	{
#if NODECPP_MEMORY_SAFETY < 0
		return;
#endif
#ifdef NODECPP_MEMORY_SAFETY_ON_DEMAND
		if ( g_CurrentAllocManager == nullptr )
			return;
#else
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, g_CurrentAllocManager != nullptr );
#endif // NODECPP_MEMORY_SAFETY_ON_DEMAND

		using T = dummy_objects::SomeWithSafePointers;

		size_t rngSeed = 155;
		dummy_objects::PRNG rng(rngSeed);
		size_t rngCheckVal = rng.rng();

		owning_ptr<dummy_objects::JustDummyStruct> someOwningPtr = make_owning<dummy_objects::JustDummyStruct>();

		owning_ptr<T> TPtr = make_owning<T>(someOwningPtr); // create an object
		TPtr->init(rngCheckVal);
		typename T::MyNonPointerMembers iniData = TPtr->getMyNonPointerMembers();

		// assume that a raw pointer is somehow obtained ...
		T* rawTPtr = &(*TPtr); 
		// ... cause object destruction ...
		TPtr.reset();
		// ... and let's see, what can be done with a zombie object pointed by it:

		// supposedly, the same state of memory
		typename T::MyNonPointerMembers postDtorData = rawTPtr->getMyNonPointerMembers();
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, iniData == postDtorData );

		// safe attempt to call a virtual member function
		rawTPtr->doSmthSome();

		bool testOK = true;

		// exception while trying to move a member owning pointer
		owning_ptr<dummy_objects::JustDummyStruct> opTarget;
		try { opTarget = std::move( rawTPtr->opDummy ); testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, opTarget == nullptr );

		// exception while trying to move a member soft pointer
		soft_ptr<dummy_objects::JustDummyStruct> spTarget;
		try { spTarget = std::move( rawTPtr->spDummy ); testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, spTarget == nullptr );

		// exception while trying to dereference a soft pointer
		try { rawTPtr->opDummy->n1 = 17; testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
		try { (*(rawTPtr->opDummy)).n1 = 17; testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
		try { rawTPtr->spDummy->n1 = 17; testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
		try { (*(rawTPtr->spDummy)).n1 = 17; testOK = false; } catch(...) {}
		NODECPP_ASSERT(::safememory::module_id, ::nodecpp::assert::AssertLevel::critical, testOK );
	}

public:
	static void checkBasics()
	{
		checkBasicsT<dummy_objects::Small>();
		checkBasicsT<dummy_objects::Large>();
		detail::killAllZombies();
	}
	static void checkSafePointers()
	{
		checkSafePointers_();
		detail::killAllZombies();
	}
};

} // namespace safememory::testing

#endif // STARTUP_CHECKS_H