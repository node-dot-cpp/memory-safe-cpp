#ifndef STARTUP_CHECKS_H
#define STARTUP_CHECKS_H

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
		FORCE_INLINE uint32_t rng32() { uint64_t ret = seedVal * 0xd989bcacc137dcd5ull; seedVal ^= seedVal >> 11; seedVal ^= seedVal << 31; seedVal ^= seedVal >> 18; return uint32_t(ret >> 32ull); }
		FORCE_INLINE uint64_t rng64() { uint64_t ret = rng32(); ret <<= 32; return ret + rng32(); }
		FORCE_INLINE uint64_t rng64NoNull() { uint64_t ret; do { ret = rng32(); ret <<= 32; ret += rng32(); } while (ret == 0); return ret; }
	};

	class SmallBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthSmall() { return 0x1;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64NoNull();}
		virtual ~SmallBase() { sn = 0;dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64();}
	};
	class SmallDerived : public SmallBase { 
	public: 
		uint64_t sn1; 
		int doSmthSmall() override {return 0x2;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64NoNull(); SmallBase::init(rng.rng64());} 
		virtual ~SmallDerived() {sn1 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64() && SmallBase::check(rng.rng64());}
	};
	class Small : public SmallDerived { 
	public: 
		uint64_t sn2; 
		int doSmthSmall() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rng64NoNull(); 
		SmallDerived::init(rng.rng64());} 
		virtual ~Small() {sn2 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng64() && SmallDerived::check(rng.rng64());}
	};

	static constexpr size_t SZ = 0x200;
	
	class LargeBase { 
	public: 
		uint64_t ln[SZ]; 
		virtual int doSmthLarge() { return 0x100;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rng64NoNull();}
		virtual ~LargeBase() { ln[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = true; for ( size_t i=0; i<SZ;++i) ret = ret && ln[i] == rng.rng64(); return ret;}
	};
	class LargeDerived : public LargeBase { 
	public: 
		uint64_t ln1[SZ]; 
		int doSmthLarge() override { return 0x200;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rng64NoNull(); LargeBase::init(rng.rng64());} 
		virtual ~LargeDerived() {ln1[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = ln1[0] == rng.rng64(); for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64(); return ret && LargeBase::check(rng.rng64());}
	};
	class Large : public LargeDerived { 
	public: 
		uint64_t ln2; 
		int doSmthLarge() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); ln2 = rng.rng64NoNull(); 
		LargeDerived::init(rng.rng64());} 
		virtual ~Large() {ln2 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return ln2 == rng.rng64() && LargeDerived::check(rng.rng64());}
	};

	enum ChangeStatus { no = 0, yes = 1, potential = 2 };

	static void checkSmall()
	{
		size_t rngSeed = 155;
		PRNG rng(rngSeed);
		uint64_t rngCheckVal;

		rngCheckVal = rng.rng64();

		uint8_t* mem4Small = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
		uint8_t* mem4SmallCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
		uint8_t* changeMap = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
		Small* SmallPtr = new ( mem4Small ) Small;
		SmallPtr->init(rngCheckVal);
		StartupCheckAssertion( SmallPtr->check(rngCheckVal));
		memcpy( mem4SmallCopy, mem4Small, sizeof(Small) );

		// collect actual addresses of changes in destructors
		uint64_t* snAddr = &(SmallPtr->sn);
		uint64_t* sn1Addr = &(SmallPtr->sn1);
		uint64_t* sn2Addr = &(SmallPtr->sn2);
		auto vmtVal = readVMT( SmallPtr );

		memset( changeMap, ChangeStatus::no, sizeof(Small) );
		memset( changeMap + ( ((uint8_t*)snAddr) - mem4Small ), ChangeStatus::yes, sizeof( uint64_t) );
		memset( changeMap + ( ((uint8_t*)sn1Addr) - mem4Small ), ChangeStatus::yes, sizeof( uint64_t) );
		memset( changeMap + ( ((uint8_t*)sn2Addr) - mem4Small ), ChangeStatus::yes, sizeof( uint64_t) );
		auto vmtPos = getVMPPos( mem4Small );
		memset( changeMap + vmtPos.first, ChangeStatus::yes, vmtPos.second );

		destruct( SmallPtr );

		if( memcmp( mem4SmallCopy, mem4Small, sizeof(Small) ) != 0 )
		{
			// check explicitly that changes done in dtor actually happened as intended (fast detection)
			StartupCheckAssertion( *snAddr == 0 );
			StartupCheckAssertion( *sn1Addr == 0 );
			StartupCheckAssertion( *sn1Addr == 0 );

			for ( size_t i=0; i<sizeof(Small); ++i )
				if ( mem4Small[i] != mem4SmallCopy[i] ) 
					StartupCheckAssertion( changeMap[i] != ChangeStatus::no );
		}
		else
			StartupCheckAssertion( false ); // our intention in dtor was to change memory state under the object

		g_AllocManager.deallocate( mem4Small );
		g_AllocManager.deallocate( mem4SmallCopy );
		g_AllocManager.deallocate( changeMap );
	}

	static void checkLarge()
	{
		size_t rngSeed = 155;
		PRNG rng(rngSeed);
		uint64_t rngCheckVal;

		rngCheckVal = rng.rng64();

		uint8_t* mem4Large = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Large)));
		uint8_t* mem4LargeCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Large)));
		uint8_t* changeMap = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Large)));
		Large* LargePtr = new ( mem4Large ) Large;
		LargePtr->init(rngCheckVal);
		StartupCheckAssertion( LargePtr->check(rngCheckVal));
		memcpy( mem4LargeCopy, mem4Large, sizeof(Large) );

		// collect actual addresses of changes in destructors
		uint64_t* lnAddr = &(LargePtr->ln[0]);
		uint64_t* ln1Addr = &(LargePtr->ln1[0]);
		uint64_t* ln2Addr = &(LargePtr->ln2);
		auto vmtVal = readVMT( LargePtr );

		memset( changeMap, ChangeStatus::no, sizeof(Large) );
		memset( changeMap + ( ((uint8_t*)lnAddr) - mem4Large ), ChangeStatus::yes, sizeof( uint64_t) );
		memset( changeMap + ( ((uint8_t*)ln1Addr) - mem4Large ), ChangeStatus::yes, sizeof( uint64_t) );
		memset( changeMap + ( ((uint8_t*)ln2Addr) - mem4Large ), ChangeStatus::yes, sizeof( uint64_t) );
		auto vmtPos = getVMPPos( mem4Large );
		memset( changeMap + vmtPos.first, ChangeStatus::yes, vmtPos.second );

		destruct( LargePtr );

		if( memcmp( mem4LargeCopy, mem4Large, sizeof(Large) ) != 0 )
		{
			// check explicitly that changes done in dtor actually happened as intended (fast detection)
			StartupCheckAssertion( *lnAddr == 0 );
			StartupCheckAssertion( *ln1Addr == 0 );
			StartupCheckAssertion( *ln1Addr == 0 );

			uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Large);
			uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4LargeCopy);

			for ( size_t i=0; i<sizeof(Large); ++i )
				if ( mem4Large[i] != mem4LargeCopy[i] ) 
					StartupCheckAssertion( changeMap[i] != ChangeStatus::no );
		}
		else
			StartupCheckAssertion( false ); // our intention in dtor was to change memory state under the object

		g_AllocManager.deallocate( mem4Large );
		g_AllocManager.deallocate( mem4LargeCopy );
		g_AllocManager.deallocate( changeMap );
	}

public:
	static void check()
	{
		checkSmall();
		checkLarge();
	}
};

#endif // STARTUP_CHECKS_H