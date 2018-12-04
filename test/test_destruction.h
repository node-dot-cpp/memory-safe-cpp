#ifndef TEST_DESTRUCTION_H
#define TEST_DESTRUCTION_H

#include "../src/safe_ptr.h"

class PRNG
{
	uint64_t seedVal;
public:
	PRNG() { seedVal = 1; }
	PRNG( size_t seed_ ) { seedVal = seed_ ? seed_ : 1; }
	void seed( size_t seed_ ) { seedVal = seed_ ? seed_ : 1; }

	FORCE_INLINE uint32_t rng32()
	{
		// based on implementation of xorshift by Arvid Gerstmann
		// see, for instance, https://arvid.io/2018/07/02/better-cxx-prng/
		uint64_t ret = seedVal * 0xd989bcacc137dcd5ull;
		seedVal ^= seedVal >> 11;
		seedVal ^= seedVal << 31;
		seedVal ^= seedVal >> 18;
		return uint32_t(ret >> 32ull);
	}

	FORCE_INLINE uint64_t rng64()
	{return 0x55;
        uint64_t ret = rng32();
		ret <<= 32;
		return ret + rng32();
	}

	FORCE_INLINE uint64_t rng64NeverNull()
	{return 0x55;
        uint64_t ret = rng32();
		ret <<= 32;
		ret += rng32();
		if ( ret == 0 ) ret = 1;
		return ret;
	}
};

void Assert( bool cond ) { if (!cond) throw std::bad_exception(); }

uint64_t dummy = 0x123456;
uint64_t* ptr1 = &dummy;
uint64_t* ptr2 = &dummy;

extern void dummyCall( void* p );

inline
void* readVMT(void* p) { return *((void**)p); }

inline
void restoreVMT(void* p, void* vpt) { *((void**)p) = vpt; }

template<class T>
void destruct( T* t )
{
	if constexpr ( std::is_polymorphic<T>::value )
	{
		auto vpt = readVMT(t);
		t->~T();
		restoreVMT( t, vpt);
	}
	else
		t->~T();
}

void testDestruction()
{
	size_t rngSeed = 155;
	PRNG rng(rngSeed);
	uint64_t rngCheckVal;

	class SmallNonVirtualBase { 
	public: 
		uint64_t sn; 
		virtual void doSmthSmall() {printf( "SmallNonVirtualBase::doSmthSmall()\n" );} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64NeverNull();}
		virtual ~SmallNonVirtualBase() { sn = 0;dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64NeverNull();}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); return sn == 0;}
	};
	class SmallVirtualBase : public SmallNonVirtualBase { 
	public: 
		uint64_t sn1; 
		void doSmthSmall() override {printf( "SmallVirtualBase::doSmthSmall()\n" );} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64NeverNull(); SmallNonVirtualBase::init(rng.rng64NeverNull());} 
		virtual ~SmallVirtualBase() {sn1 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64NeverNull() && SmallNonVirtualBase::check(rng.rng64NeverNull());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); return sn1 == 0 && SmallNonVirtualBase::checkPostDtor(rng.rng64NeverNull());}
	};
	class Small : public SmallVirtualBase { 
	public: 
		uint64_t sn2; 
		void doSmthSmall() override {printf( "Small::doSmthSmall()\n" );} 
		void init(size_t seed) {PRNG rng(seed); sn2 = rng.rng64NeverNull(); 
		SmallVirtualBase::init(rng.rng64NeverNull());} 
		virtual ~Small() {sn2 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn2 == rng.rng64NeverNull() && SmallVirtualBase::check(rng.rng64NeverNull());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); return sn2 == 0 && SmallVirtualBase::checkPostDtor(rng.rng64NeverNull());}
	};

	constexpr size_t SZ = 0x200;
	class LargeNonVirtualBase { 
	public: 
		uint64_t ln[SZ]; 
		virtual void doSmthLarge() {printf( "SLargeNonVirtualBase::doSmthLarge()\n" );} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rng64NeverNull();}
		virtual ~LargeNonVirtualBase() { ln[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = true; for ( size_t i=0; i<SZ;++i) ret = ret && ln[i] == rng.rng64NeverNull(); return ret;}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); bool ret = ln[0] == 0; for ( size_t i=1; i<SZ;++i) ret = ret && ln[i] == rng.rng64NeverNull(); return ret;}
	};
	class LargeVirtualBase : public LargeNonVirtualBase { 
	public: 
		uint64_t ln1[SZ]; 
		void doSmthLarge() override {printf( "LargeVirtualBase::doSmthLarge()\n" );} 
		void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rng64NeverNull(); LargeNonVirtualBase::init(rng.rng64NeverNull());} 
		virtual ~LargeVirtualBase() {ln1[0] = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); bool ret = ln1[0] == rng.rng64NeverNull(); for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64NeverNull(); return ret && LargeNonVirtualBase::check(rng.rng64NeverNull());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); bool ret = ln[0] == 0; for ( size_t i=1; i<SZ;++i) ret = ret && ln1[i] == rng.rng64NeverNull(); return ret && LargeNonVirtualBase::checkPostDtor(rng.rng64NeverNull());}
	};

	/*class Large : public LargeVirtualBase { 
	public: 
		uint64_t ln2; 
		int lm2; 
	};*/


	class YetNonVirtualBase { 
	public: 
		uint64_t sn; 
		virtual void doSmthYet() {printf( "YetNonVirtualBase::doSmthYet()\n" );} 
		void init(size_t seed) {PRNG rng(seed); sn = rng.rng64NeverNull();}
		virtual ~YetNonVirtualBase() { sn = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn == rng.rng64NeverNull();}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); return sn == 0;}
	};
	class YetVirtualBase : public YetNonVirtualBase { 
	public: 
		uint64_t sn1; 
		void doSmthYet() override {printf( "YetVirtualBase::doSmthYet()\n" );} 
		void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64NeverNull(); YetNonVirtualBase::init(rng.rng64NeverNull());} 
		virtual ~YetVirtualBase() {sn1 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return sn1 == rng.rng64NeverNull() && YetNonVirtualBase::check(rng.rng64NeverNull());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); return sn1 == 0 && YetNonVirtualBase::checkPostDtor(rng.rng64NeverNull());}
	};

	class Multiple : public LargeVirtualBase, public SmallVirtualBase, public YetNonVirtualBase { 
	public: 
		uint64_t mn3; 
		void doSmthSmall() override {printf( "Multiple::doSmthSmall()\n" );} 
		void doSmthLarge() override {printf( "Multiple::doSmthLarge()\n" );} 
		void init(size_t seed) {PRNG rng(seed); mn3 = rng.rng64NeverNull(); LargeVirtualBase::init(rng.rng64NeverNull()); SmallVirtualBase::init(rng.rng64NeverNull());} 
		~Multiple() { mn3 = 0; dummyCall(this);}
		bool check(size_t seed) {PRNG rng(seed); return mn3 == rng.rng64NeverNull() && LargeVirtualBase::check(rng.rng64NeverNull()) && SmallVirtualBase::check(rng.rng64NeverNull());}
		bool checkPostDtor(size_t seed) {PRNG rng(seed); rng.rng64NeverNull(); return mn3 == 0 && LargeVirtualBase::checkPostDtor(rng.rng64NeverNull()) && SmallVirtualBase::checkPostDtor(rng.rng64NeverNull());}
	};

	printf( "sizeof(SmallNonVirtualBase) = %zd\n", sizeof(SmallNonVirtualBase) );
	printf( "sizeof(SmallVirtualBase) = %zd\n", sizeof(SmallVirtualBase) );
	printf( "sizeof(Small) = %zd\n", sizeof(Small) );
	printf( "sizeof(LargeVirtualBase) = %zd\n", sizeof(LargeVirtualBase) );
	printf( "sizeof(Multiple) = %zd\n", sizeof(Multiple) );

	printf( "testing class SmallNonVirtualBase...\n" );
	rngCheckVal = rng.rng64NeverNull();
	uint8_t* mem4SmallNonVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallNonVirtualBase)));
	uint8_t* mem4SmallNonVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallNonVirtualBase)));
	SmallNonVirtualBase* SmallNonVirtualBasePtr = new ( mem4SmallNonVirtualBase ) SmallNonVirtualBase;
	SmallNonVirtualBasePtr->init(rngCheckVal);
	Assert( SmallNonVirtualBasePtr->check(rngCheckVal));
	memcpy( mem4SmallNonVirtualBaseCopy, mem4SmallNonVirtualBase, sizeof(SmallNonVirtualBase) );
	SmallNonVirtualBasePtr->~SmallNonVirtualBase();
	if ( SmallNonVirtualBasePtr->check(rngCheckVal) )
		printf( "writing in dtor is opt out\n" );
	else
	{
		Assert( SmallNonVirtualBasePtr->checkPostDtor(rngCheckVal));
		Assert( !SmallNonVirtualBasePtr->check(rngCheckVal));
	}
	if ( memcmp( mem4SmallNonVirtualBaseCopy, mem4SmallNonVirtualBase, sizeof(SmallNonVirtualBase) ) != 0 )
	{
		printf( "class SmallNonVirtualBase memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4SmallNonVirtualBase);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallNonVirtualBaseCopy);
		for ( size_t i=0; i<sizeof(SmallNonVirtualBase) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
		for ( size_t i=0; i<sizeof(SmallNonVirtualBase) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		printf( "\n" );
	}

	printf( "testing class SmallVirtualBase...\n" );
	rngCheckVal = rng.rng64NeverNull();
	uint8_t* mem4SmallVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	uint8_t* mem4SmallVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	SmallVirtualBase* SmallVirtualBasePtr = new ( mem4SmallVirtualBase ) SmallVirtualBase;
	SmallVirtualBasePtr->init(rngCheckVal);
	Assert( SmallVirtualBasePtr->check(rngCheckVal));
	memcpy( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) );
	//SmallVirtualBasePtr->~SmallVirtualBase();
	destruct( SmallVirtualBasePtr );
	if ( SmallVirtualBasePtr->check(rngCheckVal) )
		printf( "writing in dtor is opt out\n" );
	else
	{
		Assert( SmallVirtualBasePtr->checkPostDtor(rngCheckVal));
		Assert( !SmallVirtualBasePtr->check(rngCheckVal));
	}
	if ( memcmp( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) ) != 0 )
	{
		printf( "class SmallVirtualBase memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4SmallVirtualBase);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallVirtualBaseCopy);
		for ( size_t i=0; i<sizeof(SmallVirtualBase) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
		for ( size_t i=0; i<sizeof(SmallVirtualBase) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		printf( "\n" );
	}

	printf( "testing class Small...\n" );
	rngCheckVal = rng.rng64NeverNull();
	uint8_t* mem4Small = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	uint8_t* mem4SmallCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	Small* SmallPtr = new ( mem4Small ) Small;
	SmallPtr->init(rngCheckVal);
	Assert( SmallPtr->check(rngCheckVal));
	memcpy( mem4SmallCopy, mem4Small, sizeof(Small) );

	//SmallPtr->~Small();
	destruct( SmallPtr );

	if ( SmallPtr->check(rngCheckVal) )
		printf( "writing in dtor is opt out\n" );
	else
	{
		Assert( SmallPtr->checkPostDtor(rngCheckVal));
		Assert( !SmallPtr->check(rngCheckVal));
	}
	if( memcmp( mem4SmallCopy, mem4Small, sizeof(Small) ) != 0 )
	{
		printf( "class Small memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Small);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallCopy);
		for ( size_t i=0; i<sizeof(Small) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
		for ( size_t i=0; i<sizeof(Small) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				printf( "%zd: %llx -> %llx\n", i, p64Copy[i], p64[i] );
		printf( "\n" );
	}

	printf( "testing class Multiple...\n" );
	rngCheckVal = rng.rng64NeverNull();
	uint8_t* mem4Multiple = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	uint8_t* mem4MultipleCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	Multiple* MultiplePtr = new ( mem4Multiple ) Multiple;
	SmallNonVirtualBase* secondBase = MultiplePtr;
	MultiplePtr->init(rngCheckVal);
	Assert( MultiplePtr->check(rngCheckVal));
	memcpy( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) );
	ptr1 = (uint64_t*)mem4Multiple;
	ptr2 = ((uint64_t*)mem4Multiple) + sizeof(LargeVirtualBase) / sizeof(uint64_t);
	//auto vpt = readVMT(MultiplePtr);
	//MultiplePtr->~Multiple();
	//restoreVMT( MultiplePtr, vpt);
	destruct( MultiplePtr );
	if ( MultiplePtr->check(rngCheckVal) )
		printf( "writing in dtor is opt out\n" );
	else
	{
		Assert( MultiplePtr->checkPostDtor(rngCheckVal));
		Assert( !MultiplePtr->check(rngCheckVal));
	}
	if( memcmp( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) ) != 0 )
	{
		printf( "class Multiple memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Multiple);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4MultipleCopy);
		for ( size_t i=0; i<sizeof(Multiple) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
		for ( size_t i=0; i<sizeof(Multiple) / sizeof(uint64_t); ++i )
			if ( p64[i] != p64Copy[i] )
				printf( "%zd: 0x%llx -> 0x%llx\n", i, p64Copy[i], p64[i] );
		printf( "this = 0x%llx\n", (uintptr_t)MultiplePtr );
		printf( "(LargeVirtualBase*)this = 0x%llx (+%lld)\n", (uintptr_t)((LargeVirtualBase*)MultiplePtr), ( (uintptr_t)((LargeVirtualBase*)MultiplePtr) - (uintptr_t)MultiplePtr ) / sizeof( uint64_t) );
		printf( "(SmallVirtualBase*)this = 0x%llx (+%lld)\n", (uintptr_t)((SmallVirtualBase*)MultiplePtr), ( (uintptr_t)((SmallVirtualBase*)MultiplePtr) - (uintptr_t)MultiplePtr ) / sizeof( uint64_t) );
		printf( "\n" );
	}
	// now let's try to access MultiplePtr
	printf( "accessing virtual method of the first base:\n" );
	MultiplePtr->doSmthLarge();
	printf( "[1]accessing virtual method of the second base:\n" );
	secondBase->doSmthSmall();
	printf( "[2]accessing virtual method of the second base:\n" );
	SmallNonVirtualBase* secondBase2 = MultiplePtr;
	secondBase2->doSmthSmall();
	printf( "[3] accessing virtual method of the second base:\n" );
	MultiplePtr->doSmthSmall();
	printf( "[3] accessing virtual method of the third base:\n" );
	MultiplePtr->doSmthYet();

	printf( "testDestruction(): OK\n\n" );
}



#endif
