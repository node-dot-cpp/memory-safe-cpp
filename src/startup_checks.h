#ifndef STARTUP_CHECKS_H
#define STARTUP_CHECKS_H

void StartupCheckAssertion( bool cond ) { if (!cond) throw std::bad_exception(); } // TODO: replace by means standard for the project

inline
void testDestructionMeans( bool verbose = false )
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
	};
	size_t rngSeed = 155;
	PRNG rng(rngSeed);
	uint64_t rngCheckVal;

	class SmallSuperVirtualBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthSmall() { return 0x1;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64();}
		virtual ~SmallSuperVirtualBase() { sn = 0;dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64();}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); return sn == 0;}
	};
	class SmallVirtualBase : public SmallSuperVirtualBase { 
	public: 
		uint64_t sn1; 
		int doSmthSmall() override {return 0x2;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64(); SmallSuperVirtualBase::init(rng.rng64());} 
		virtual ~SmallVirtualBase() {sn1 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64() && SmallSuperVirtualBase::check(rng.rng64());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); return sn1 == 0 && SmallSuperVirtualBase::checkPostDtor(rng.rng64());}
	};
	class Small : public SmallVirtualBase { 
	public: 
		uint64_t sn2; 
		int doSmthSmall() override { return 0x3; } 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rng64(); 
		SmallVirtualBase::init(rng.rng64());} 
		virtual ~Small() {sn2 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng64() && SmallVirtualBase::check(rng.rng64());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); return sn2 == 0 && SmallVirtualBase::checkPostDtor(rng.rng64());}
	};

	constexpr size_t SZ = 0x200;
	class LargeSuperVirtualBase { 
	public: 
		uint64_t ln[SZ]; 
		virtual int doSmthLarge() { return 0x100;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rng64();}
		virtual ~LargeSuperVirtualBase() { ln[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = true; for ( size_t i=0; i<SZ;++i) ret = ret && ln[i] == rng.rng64(); return ret;}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); bool ret = ln[0] == 0; for ( size_t i=1; i<SZ;++i) ret = ret && ln[i] == rng.rng64(); return ret;}
	};
	class LargeVirtualBase : public LargeSuperVirtualBase { 
	public: 
		uint64_t ln1[SZ]; 
		int doSmthLarge() override { return 0x200;} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rng64(); LargeSuperVirtualBase::init(rng.rng64());} 
		virtual ~LargeVirtualBase() {ln1[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = ln1[0] == rng.rng64(); for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64(); return ret && LargeSuperVirtualBase::check(rng.rng64());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); bool ret = ln[0] == 0; for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64(); return ret && LargeSuperVirtualBase::checkPostDtor(rng.rng64());}
	};


	class YetSuperVirtualBase { 
	public: 
		uint64_t sn; 
		virtual int doSmthYet() { return 0x10000;} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64();}
		virtual ~YetSuperVirtualBase() { sn = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64();}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); return sn == 0;}
	};
	class YetVirtualBase : public YetSuperVirtualBase { 
	public: 
		uint64_t sn1; 
		int doSmthYet() override { return 0x20000;} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64(); YetSuperVirtualBase::init(rng.rng64());} 
		virtual ~YetVirtualBase() {sn1 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64() && YetSuperVirtualBase::check(rng.rng64());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); return sn1 == 0 && YetSuperVirtualBase::checkPostDtor(rng.rng64());}
	};

	class Multiple : public LargeVirtualBase, public SmallVirtualBase, public YetSuperVirtualBase { 
	public: 
		uint64_t mn3; 
		int doSmthSmall() override { return 0x3;} 
		int doSmthLarge() override { return 0x300;} 
		int doSmthYet() override { return 0x30000;} 
		void init(size_t seed) {PRNG rng(seed); mn3 = rng.rng64(); LargeVirtualBase::init(rng.rng64()); SmallVirtualBase::init(rng.rng64());} 
		~Multiple() { mn3 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return mn3 == rng.rng64() && LargeVirtualBase::check(rng.rng64()) && SmallVirtualBase::check(rng.rng64());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64(); return mn3 == 0 && LargeVirtualBase::checkPostDtor(rng.rng64()) && SmallVirtualBase::checkPostDtor(rng.rng64());}
	};

	if ( verbose ) printf( "sizeof(SmallSuperVirtualBase) = %zd\n", sizeof(SmallSuperVirtualBase) );
	if ( verbose ) printf( "sizeof(SmallVirtualBase) = %zd\n", sizeof(SmallVirtualBase) );
	if ( verbose ) printf( "sizeof(Small) = %zd\n", sizeof(Small) );
	if ( verbose ) printf( "sizeof(LargeVirtualBase) = %zd\n", sizeof(LargeVirtualBase) );
	if ( verbose ) printf( "sizeof(Multiple) = %zd\n", sizeof(Multiple) );

	if ( verbose ) printf( "testing class SmallSuperVirtualBase...\n" );
	rngCheckVal = rng.rng64();
	uint8_t* mem4SmallSuperVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallSuperVirtualBase)));
	uint8_t* mem4SmallSuperVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallSuperVirtualBase)));
	SmallSuperVirtualBase* SmallSuperVirtualBasePtr = new ( mem4SmallSuperVirtualBase ) SmallSuperVirtualBase;
	SmallSuperVirtualBasePtr->init(rngCheckVal);
	StartupCheckAssertion( SmallSuperVirtualBasePtr->check(rngCheckVal));
	memcpy( mem4SmallSuperVirtualBaseCopy, mem4SmallSuperVirtualBase, sizeof(SmallSuperVirtualBase) );
	SmallSuperVirtualBasePtr->~SmallSuperVirtualBase();
	if ( SmallSuperVirtualBasePtr->check(rngCheckVal) )
		if ( verbose ) printf( "writing in dtor is opt out\n" );
	else
	{
		StartupCheckAssertion( SmallSuperVirtualBasePtr->checkPostDtor(rngCheckVal));
		StartupCheckAssertion( !SmallSuperVirtualBasePtr->check(rngCheckVal));
	}
	if ( memcmp( mem4SmallSuperVirtualBaseCopy, mem4SmallSuperVirtualBase, sizeof(SmallSuperVirtualBase) ) != 0 )
	{
		if ( verbose ) printf( "class SmallSuperVirtualBase memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4SmallSuperVirtualBase);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallSuperVirtualBaseCopy);
		for ( size_t i=0; i<sizeof(SmallSuperVirtualBase) / sizeof(uint64_t); ++i )
			if ( verbose ) printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		if ( verbose ) printf( "\n" );
		for ( size_t i=0; i<sizeof(SmallSuperVirtualBase) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				if ( verbose ) printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		if ( verbose ) printf( "\n" );
	}
	g_AllocManager.deallocate( mem4SmallSuperVirtualBase );
	g_AllocManager.deallocate( mem4SmallSuperVirtualBaseCopy );

	if ( verbose ) printf( "testing class SmallVirtualBase...\n" );
	rngCheckVal = rng.rng64();
	uint8_t* mem4SmallVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	uint8_t* mem4SmallVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	SmallVirtualBase* SmallVirtualBasePtr = new ( mem4SmallVirtualBase ) SmallVirtualBase;
	SmallVirtualBasePtr->init(rngCheckVal);
	StartupCheckAssertion( SmallVirtualBasePtr->check(rngCheckVal));
	memcpy( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) );
	//SmallVirtualBasePtr->~SmallVirtualBase();
	destruct( SmallVirtualBasePtr );
	if ( SmallVirtualBasePtr->check(rngCheckVal) )
		if ( verbose ) printf( "writing in dtor is opt out\n" );
	else
	{
		StartupCheckAssertion( SmallVirtualBasePtr->checkPostDtor(rngCheckVal));
		StartupCheckAssertion( !SmallVirtualBasePtr->check(rngCheckVal));
	}
	if ( memcmp( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) ) != 0 )
	{
		if ( verbose ) printf( "class SmallVirtualBase memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4SmallVirtualBase);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallVirtualBaseCopy);
		for ( size_t i=0; i<sizeof(SmallVirtualBase) / sizeof(uint64_t); ++i )
			if ( verbose ) printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		if ( verbose ) printf( "\n" );
		for ( size_t i=0; i<sizeof(SmallVirtualBase) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				if ( verbose ) printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		if ( verbose ) printf( "\n" );
	}
	g_AllocManager.deallocate( mem4SmallVirtualBase );
	g_AllocManager.deallocate( mem4SmallVirtualBaseCopy );

	if ( verbose ) printf( "testing class Small...\n" );
	rngCheckVal = rng.rng64();
	uint8_t* mem4Small = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	uint8_t* mem4SmallCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	Small* SmallPtr = new ( mem4Small ) Small;
	SmallPtr->init(rngCheckVal);
	StartupCheckAssertion( SmallPtr->check(rngCheckVal));
	memcpy( mem4SmallCopy, mem4Small, sizeof(Small) );

	//SmallPtr->~Small();
	destruct( SmallPtr );

	if ( SmallPtr->check(rngCheckVal) )
		if ( verbose ) printf( "writing in dtor is opt out\n" );
	else
	{
		StartupCheckAssertion( SmallPtr->checkPostDtor(rngCheckVal));
		StartupCheckAssertion( !SmallPtr->check(rngCheckVal));
	}
	if( memcmp( mem4SmallCopy, mem4Small, sizeof(Small) ) != 0 )
	{
		if ( verbose ) printf( "class Small memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Small);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallCopy);
		for ( size_t i=0; i<sizeof(Small) / sizeof(uint64_t); ++i )
			if ( verbose ) printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		if ( verbose ) printf( "\n" );
		for ( size_t i=0; i<sizeof(Small) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				if ( verbose ) printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		if ( verbose ) printf( "\n" );
	}
	g_AllocManager.deallocate( mem4Small );
	g_AllocManager.deallocate( mem4SmallCopy );

	if ( verbose ) printf( "testing class Multiple...\n" );
	rngCheckVal = rng.rng64();
	uint8_t* mem4Multiple = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	uint8_t* mem4MultipleCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	Multiple* MultiplePtr = new ( mem4Multiple ) Multiple;
	SmallSuperVirtualBase* secondBase = MultiplePtr;
	MultiplePtr->init(rngCheckVal);
	StartupCheckAssertion( MultiplePtr->check(rngCheckVal));
	memcpy( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) );
	//auto vpt = readVMT(MultiplePtr);
	//MultiplePtr->~Multiple();
	//restoreVMT( MultiplePtr, vpt);
	destruct( MultiplePtr );
	if ( MultiplePtr->check(rngCheckVal) )
		if ( verbose ) printf( "writing in dtor is opt out\n" );
	else
	{
		StartupCheckAssertion( MultiplePtr->checkPostDtor(rngCheckVal));
		StartupCheckAssertion( !MultiplePtr->check(rngCheckVal));
	}
	if( memcmp( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) ) != 0 )
	{
		if ( verbose ) printf( "class Multiple memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Multiple);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4MultipleCopy);
		for ( size_t i=0; i<sizeof(Multiple) / sizeof(uint64_t); ++i )
			if ( verbose ) printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		if ( verbose ) printf( "\n" );
		for ( size_t i=0; i<sizeof(Multiple) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				if ( verbose ) printf( "%zd: 0x%llx -> 0x%llx\n", i, p64Copy[i], p64[i] );
		if ( verbose ) printf( "this = 0x%llx\n", (uintptr_t)MultiplePtr );
		if ( verbose ) printf( "(LargeVirtualBase*)this = 0x%llx (+%lld)\n", (uintptr_t)((LargeVirtualBase*)MultiplePtr), ( (uintptr_t)((LargeVirtualBase*)MultiplePtr) - (uintptr_t)MultiplePtr ) / sizeof( uint64_t) );
		if ( verbose ) printf( "(SmallVirtualBase*)this = 0x%llx (+%lld)\n", (uintptr_t)((SmallVirtualBase*)MultiplePtr), ( (uintptr_t)((SmallVirtualBase*)MultiplePtr) - (uintptr_t)MultiplePtr ) / sizeof( uint64_t) );
		if ( verbose ) printf( "\n" );
	}
	// now let's try to access MultiplePtr
	int callOutput = 0;
	if ( verbose ) printf( "accessing virtual method of the first base:\n" );
	if ( verbose ) printf( "[1]accessing virtual method of the second base:\n" );
	secondBase->doSmthSmall();
	if ( verbose ) printf( "[2]accessing virtual method of the second base:\n" );
	SmallSuperVirtualBase* secondBase2 = MultiplePtr;
	secondBase2->doSmthSmall();
	if ( verbose ) printf( "[3] accessing virtual method of the second base:\n" );
	if ( verbose ) printf( "[3] accessing virtual method of the third base:\n" );
	callOutput += MultiplePtr->doSmthLarge();
	callOutput += MultiplePtr->doSmthSmall();
	callOutput += MultiplePtr->doSmthYet();
	if ( verbose ) printf( "callOutput = 0x%x\n", callOutput );

	g_AllocManager.deallocate( mem4Multiple );
	g_AllocManager.deallocate( mem4MultipleCopy );

	if ( verbose ) printf( "testDestruction(): OK\n\n" );
}

#endif // STARTUP_CHECKS_H