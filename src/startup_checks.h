#ifndef STARTUP_CHECKS_H
#define STARTUP_CHECKS_H

#include "foundation/include/foundation.h"

void StartupCheckAssertion( bool cond ) { if (!cond) throw std::bad_exception(); } // TODO: replace by means standard for the project

class StartupChecker
{
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
	};

	class SmallBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthSmall() { return 0x1;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64NoNull();}
		virtual ~SmallBase() { sn = 0;forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64();}
	};
	class SmallDerived : public SmallBase { 
	public: 
		uint64_t sn1; 
		int doSmthSmall() override {return 0x2;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64NoNull(); SmallBase::init(rng.rng64());} 
		virtual ~SmallDerived() {sn1 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64() && SmallBase::check(rng.rng64());}
	};
	class Small : public SmallDerived { 
	public: 
		uint64_t sn2; 
		int doSmthSmall() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rng64NoNull(); 
		SmallDerived::init(rng.rng64());} 
		virtual ~Small() {sn2 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng64() && SmallDerived::check(rng.rng64());}
	};

	static constexpr size_t SZ = 0x200;
	
	class LargeBase { 
	public: 
		uint64_t ln[SZ]; 
		virtual int doSmthLarge() { return 0x100;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rng64NoNull();}
		virtual ~LargeBase() { ln[0] = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = true; for ( size_t i=0; i<SZ;++i) ret = ret && ln[i] == rng.rng64(); return ret;}
	};
	class LargeDerived : public LargeBase { 
	public: 
		uint64_t ln1[SZ]; 
		int doSmthLarge() override { return 0x200;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rng64NoNull(); LargeBase::init(rng.rng64());} 
		virtual ~LargeDerived() {ln1[0] = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = ln1[0] == rng.rng64(); for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64(); return ret && LargeBase::check(rng.rng64());}
	};
	class Large : public LargeDerived { 
	public: 
		uint64_t ln2; 
		int doSmthLarge() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); ln2 = rng.rng64NoNull(); 
		LargeDerived::init(rng.rng64());} 
		virtual ~Large() {ln2 = 0; forcePreviousChangesToThisInDtor(this);}
		bool check(size_t seed) {PRNG rng(seed); return ln2 == rng.rng64() && LargeDerived::check(rng.rng64());}
	};

	enum ChangeStatus { no = 0, yes = 1, potential = 2 };

	static void setAddressesOfChanges( Small* obj, uint64_t** addr1, uint64_t** addr2, uint64_t** addr3 )
	{
		*addr1 = &(obj->sn);
		*addr2 = &(obj->sn1);
		*addr3 = &(obj->sn2);
	}

	static void setAddressesOfChanges( Large* obj, uint64_t** addr1, uint64_t** addr2, uint64_t** addr3 )
	{
		*addr1 = &(obj->ln[0]);
		*addr2 = &(obj->ln1[0]);
		*addr3 = &(obj->ln2);
	}

	template<class T>
	static void checkT()
	{
		size_t rngSeed = 155;
		PRNG rng(rngSeed);
		uint64_t rngCheckVal;

		rngCheckVal = rng.rng64();

#ifdef USE_IIBMALLOC
		uint8_t* mem4T = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(T)));
		uint8_t* mem4TCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(T)));
		uint8_t* changeMap = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(T)));
		T* TPtr = new ( mem4T ) T;
		TPtr->init(rngCheckVal);
		StartupCheckAssertion( TPtr->check(rngCheckVal));
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
			StartupCheckAssertion( *addr1 == 0 );
			StartupCheckAssertion( *addr2 == 0 );
			StartupCheckAssertion( *addr3 == 0 );

			for ( size_t i=0; i<sizeof(T); ++i )
				if ( mem4T[i] != mem4TCopy[i] ) 
					StartupCheckAssertion( changeMap[i] != ChangeStatus::no );
		}
		else
			StartupCheckAssertion( false ); // our intention in dtor was to change memory state under the object

		g_AllocManager.deallocate( mem4T );
		g_AllocManager.deallocate( mem4TCopy );
		g_AllocManager.deallocate( changeMap );
#else
//#error not implemented (but implementation for any other allocator (or generalization) should not become a greate task anyway)
#endif
	}

public:
	static void check()
	{
		checkT<Small>();
		checkT<Large>();
	}
};

#endif // STARTUP_CHECKS_H