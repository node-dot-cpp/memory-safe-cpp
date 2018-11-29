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
	{
        uint64_t ret = rng32();
		ret <<= 32;
		return ret + rng32();
	}
};

void testDestruction()
{
	PRNG rng(0);

	class SmallNonVirtualBase { public: uint64_t sn; virtual void doSmthSmall() {printf( "SmallNonVirtualBase::doSmthSmall()\n" );} void init(size_t seed) {PRNG rng(seed); sn = rng.rng64();} };
	class SmallVirtualBase : public SmallNonVirtualBase { public: uint64_t sn1; void doSmthSmall() override {printf( "SmallVirtualBase::doSmthSmall()\n" );} void init(size_t seed) {PRNG rng(seed); sn1 = rng.rng64(); SmallNonVirtualBase::init(rng.rng64());} virtual ~SmallVirtualBase() {}};
	class Small : public SmallVirtualBase { public: uint64_t sn2; void doSmthSmall() override {printf( "Small::doSmthSmall()\n" );} void init(size_t seed) {PRNG rng(seed); sn2 = rng.rng64(); SmallVirtualBase::init(rng.rng64());} virtual ~Small() {} };

	constexpr size_t SZ = 0x200;
	class LargeNonVirtualBase { public: uint64_t ln[SZ]; virtual void doSmthLarge() {printf( "SLargeNonVirtualBase::doSmthLarge()\n" );} void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln[i] = rng.rng64();} };
	class LargeVirtualBase : public LargeNonVirtualBase { public: uint64_t ln1[SZ]; void doSmthLarge() override {printf( "LargeVirtualBase::doSmthLarge()\n" );} void init(size_t seed) {PRNG rng(seed); for ( size_t i=0; i<SZ;++i) ln1[i] = rng.rng64(); LargeNonVirtualBase::init(rng.rng64());} virtual ~LargeVirtualBase() {}};
	class Large : public LargeVirtualBase { public: uint64_t ln2; int lm2; };

	class Multiple : public LargeVirtualBase, SmallVirtualBase { public: uint64_t mn3; void doSmthSmall() override {printf( "Multiple::doSmthSmall()\n" );} void doSmthLarge() override {printf( "Multiple::doSmthLarge()\n" );} void init(size_t seed) {PRNG rng(seed); mn3 = rng.rng64(); LargeVirtualBase::init(rng.rng64()); SmallVirtualBase::init(rng.rng64());} };

	printf( "sizeof(SmallNonVirtualBase) = %zd\n", sizeof(SmallNonVirtualBase) );
	printf( "sizeof(SmallVirtualBase) = %zd\n", sizeof(SmallVirtualBase) );
	printf( "sizeof(Small) = %zd\n", sizeof(Small) );
	printf( "sizeof(Multiple) = %zd\n", sizeof(Multiple) );

	uint8_t* mem4SmallNonVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallNonVirtualBase)));
	uint8_t* mem4SmallNonVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallNonVirtualBase)));
	SmallNonVirtualBase* SmallNonVirtualBasePtr = new ( mem4SmallNonVirtualBase ) SmallNonVirtualBase;
	SmallNonVirtualBasePtr->init(rng.rng64());
	memcpy( mem4SmallNonVirtualBaseCopy, mem4SmallNonVirtualBase, sizeof(SmallNonVirtualBase) );
	SmallNonVirtualBasePtr->~SmallNonVirtualBase();
	assert( memcmp( mem4SmallNonVirtualBaseCopy, mem4SmallNonVirtualBase, sizeof(SmallNonVirtualBase) ) == 0 );

	uint8_t* mem4SmallVirtualBase = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	uint8_t* mem4SmallVirtualBaseCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(SmallVirtualBase)));
	SmallVirtualBase* SmallVirtualBasePtr = new ( mem4SmallVirtualBase ) SmallVirtualBase;
	SmallVirtualBasePtr->init(rng.rng64());
	memcpy( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) );
	SmallVirtualBasePtr->~SmallVirtualBase();
	assert( memcmp( mem4SmallVirtualBaseCopy, mem4SmallVirtualBase, sizeof(SmallVirtualBase) ) == 0 );

	uint8_t* mem4Small = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	uint8_t* mem4SmallCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Small)));
	Small* SmallPtr = new ( mem4Small ) Small;
	SmallPtr->init(rng.rng64());
	memcpy( mem4SmallCopy, mem4Small, sizeof(Small) );
	SmallPtr->~Small();
	if( memcmp( mem4SmallCopy, mem4Small, sizeof(Small) ) != 0 )
	{
		printf( "class Small memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Small);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4SmallCopy);
		for ( size_t i=0; i<sizeof(Small) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
	}

	uint8_t* mem4Multiple = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	uint8_t* mem4MultipleCopy = reinterpret_cast<uint8_t*>(g_AllocManager.allocate(sizeof(Multiple)));
	Multiple* MultiplePtr = new ( mem4Multiple ) Multiple;
	MultiplePtr->init(rng.rng64());
	memcpy( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) );
	MultiplePtr->~Multiple();
	if( memcmp( mem4MultipleCopy, mem4Multiple, sizeof(Multiple) ) != 0 )
	{
		printf( "class Multiple memdiff after dtor:\n" );
		uint64_t* p64 = reinterpret_cast<uint64_t*>(mem4Multiple);
		uint64_t* p64Copy = reinterpret_cast<uint64_t*>(mem4MultipleCopy);
		for ( size_t i=0; i<sizeof(Multiple) / sizeof(uint64_t); ++i )
			printf( "%c", p64[i] == p64Copy[i] ? '1' : '0' );
		printf( "\n" );
	}
	// now let's try to access MultiplePtr
	MultiplePtr->doSmthSmall();
	MultiplePtr->doSmthLarge();

	printf( "testDestruction(): OK\n\n" );
}



#endif
