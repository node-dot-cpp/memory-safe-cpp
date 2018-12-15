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

// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#include "../src/safe_ptr.h"
#include "../src/startup_checks.h"
#include "../3rdparty/lest/include/lest/lest.hpp"
#include "test_nullptr_access.h"

#ifdef SAFE_PTR_DEBUG_MODE
thread_local size_t onStackSafePtrCreationCount; 
thread_local size_t onStackSafePtrDestructionCount;
#endif // SAFE_PTR_DEBUG_MODE

#ifdef USE_IIBMALLOC
class IIBMallocInitializer
{
public:
	IIBMallocInitializer()
	{
		g_AllocManager.initialize();
		g_AllocManager.enable();
	}
	~IIBMallocInitializer()
	{
	printf( "   ===>>onStackSafePtrCreationCount = %zd, onStackSafePtrDestructionCount = %zd\n", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//assert( onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
	}
};
static IIBMallocInitializer iibmallocinitializer;
#elif defined USE_NEW_DELETE_ALLOC
thread_local void** zombieList_ = nullptr;
#endif // SAFE_PTR_DEBUG_MODE

// testing on-stack ptr detection
int g_int;
thread_local int th_int;

// testing stack optimization
owning_ptr<int> gop;
soft_ptr<int> gsp;
void fn1( soft_ptr<int> sp ) { gsp = sp; }
void fn2( soft_ptr<int> sp ) { fn1(sp); }
void fn3( soft_ptr<int> sp ) { fn2(sp); }
void fn4( soft_ptr<int> sp ) { fn3(sp); }
void fn5( soft_ptr<int> sp ) { fn4(sp); }
void fn6( soft_ptr<int> sp ) { fn5(sp); }
void fn7( soft_ptr<int> sp ) { fn6(sp); }
void fn8( soft_ptr<int> sp ) { fn7(sp); }
void fnStart() { 
	owning_ptr<int> op = make_owning<int>(); 
	*op = 17;
	soft_ptr<int> sp = op; 
	gop = std::move(op); 
	fn8(sp); 
}
void fnSoftEnd() { 
	assert( *gsp == 17 );
	soft_ptr<int> sp = std::move(gsp); 
}
void fnOwningEnd() { 
	assert( *gop == 17 );
	gsp.reset();
	gop.reset(); 
}

#if 1
int testWithLest( int argc, char * argv[] )
{
	const lest::test specification[] =
	{
		CASE( "testing pointers-with-data" )
		{
			SETUP("testing pointers-with-data")
			{
	#if 0 // TODO: rework for new data structures
				int* n1 = new int;
				int* n2 = new int;
			 //printf( "[1] n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
				Ptr2PtrWishData n1D, n2D;
				//SECTION( "initializing ptrs-with-data" )
				{
					n1D.init(n1,Ptr2PtrWishData::invalidData);
					n2D.init(n2,Ptr2PtrWishData::invalidData);
				 //printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //printf( "[2] n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
					EXPECT( n1D.getPtr() == n1 );
					EXPECT( n2D.getPtr() == n2 );
					EXPECT( n1D.getData() == Ptr2PtrWishData::invalidData );
					EXPECT( n2D.getData() == Ptr2PtrWishData::invalidData );
			 //printf( "[3] n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
				}
				//SECTION( "updating data" )
				{
			 //printf( "[4] n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
					n1D.updateData(6);
					n2D.updateData(500000);
				 //printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //printf( "[5] n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
					EXPECT( n1D.getPtr() == n1 );
					EXPECT( n2D.getPtr() == n2 );
					EXPECT( n1D.getData() == 6 );
					EXPECT( n2D.getData() == 500000 );
				}
				//SECTION( "updating pointers" )
				{
					n1D.updatePtr(n2);
					n2D.updatePtr(n1);
					EXPECT( n1D.getPtr() == n2 );
					EXPECT( n2D.getPtr() == n1 );
					EXPECT( n1D.getData() == 6 );
					EXPECT( n2D.getData() == 500000 );
					//printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				//SECTION( "yet updating data" )
				{
					n1D.updateData(500000);
					n2D.updateData(6);
					EXPECT( n1D.getPtr() == n2 );
					EXPECT( n2D.getPtr() == n1 );
					EXPECT( n1D.getData() == 500000 );
					EXPECT( n2D.getData() == 6 );
					//printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				delete n1;
				delete n2;
	#endif // 0
			}
		},

		CASE( "basic safe pointer test" )
		{
			//EXPECT_NO_THROW( basicSafePointerTest() );
			SETUP("basic safe pointer test")
			{
				soft_ptr<int> s01;
				soft_ptr<int> s02;
				SECTION( "narrowing scope [1]" )
				{
					owning_ptr<int> p1 = make_owning<int>();
					*p1 = 5;
					owning_ptr<int> p2 = make_owning<int>();
					*p2 = 25;
					soft_ptr<int> s11(p1);
					soft_ptr<int> s12(p1);
					soft_ptr<int> s21(p2);
					soft_ptr<int> s22(p2);
					*s11.get() += 1;
					*s22.get() += 1;

					EXPECT( *s11 == 6 );
					EXPECT( *s12 == 6 );
					EXPECT( *s21 == 26 );
					EXPECT( *s22 == 26 );
					//printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
					s21.swap(s12);
					//soft_ptr<int> tmp1 = s21; s21 = s12; s12 = tmp1;
 					EXPECT( *s11 == 6 );
					EXPECT( *s12 == 26 );
					EXPECT( *s21 == 6 );
					EXPECT( *s22 == 26 );
					//printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
					s01.swap(s11);
					//soft_ptr<int> tmp2 = s01; s01 = s11; s11 = tmp2;
 					//printf( "*s14 = %d\n", *s14.get() );
 					EXPECT( *s01 == 6 );
					soft_ptr<int> s13(p1);
					soft_ptr<int> s14(p1);
					{
						soft_ptr<int> s15(p1);
 						//printf( "*s15 = %d\n", *s15.get() );
						EXPECT( *s15 == 6 );
					}
					soft_ptr<int> s15(p1);
					EXPECT( *s15 == 6 );
 					//printf( "*s15 = %d\n", *s15.get() );
					soft_ptr<int> s16(p1);
					{
						soft_ptr<int> s17(p1);
						EXPECT( *s17 == 6 );
 						//printf( "*s17 = %d\n", *s17.get() );
					}
					EXPECT( *p1 == 6 );
					EXPECT( *p2 == 26 );
					//printf( "*p1 = %d, *p2 = %d\n", *p1, *p2 );
					owning_ptr<int> p3 = make_owning<int>();
					*p3 = 17;
					s02 = p3;
					EXPECT( *s02 == 17 );
				}
				//printf( "is s14 == NULL (as it shoudl be)? %s\n", s14 ? "NO" : "YES" );
				//EXPECT( !s01 );
				//EXPECT( !s02 );
			}
		},

		CASE( "basic safe pointer test" )
		{
			SETUP("basic safe pointer test")
			{
				class Base { public: int n; virtual ~Base(){} };
				class Derived : public Base { public: virtual ~Derived(){} };
				owning_ptr<Derived> p = make_owning<Derived>();
				p->n = 11;
				soft_ptr<Base> p1 = p;
				soft_ptr<Derived> p2 = soft_ptr_static_cast<Derived>(p1);
				soft_ptr<Derived> p3 = soft_ptr_reinterpret_cast<Derived>(p1);
				EXPECT( p2->n == 11 );
				EXPECT( p3->n == 11 );
			}
		},

		CASE( "test Pointers-To-Members" )
		{
			SETUP("basic safe pointer test")
			{
				class SmallNonVirtualBase { public: int sn; int sm;};
				class SmallVirtualBase : public SmallNonVirtualBase { public: int sn1; int sm1; virtual ~SmallVirtualBase() {}};
				class Small : public SmallVirtualBase { public: int sn2; int sm2;};

				class LargeNonVirtualBase { public: int ln[0x10000]; int lm;};
				class LargeVirtualBase : public LargeNonVirtualBase { public: int ln1; int lm1[0x10000]; virtual ~LargeVirtualBase() {}};
				class Large : public LargeVirtualBase { public: int ln2; int lm2;};

				class Multiple : public LargeVirtualBase, SmallVirtualBase { public: int mn3; int mm3;};

				owning_ptr<Small> pSmall = make_owning<Small>();
				soft_ptr<Small> spSmall(pSmall);
				soft_ptr<SmallVirtualBase> spSmallVirtualBase = soft_ptr_reinterpret_cast<SmallVirtualBase>( spSmall );
				soft_ptr<SmallNonVirtualBase> spSmallNonVirtualBase = soft_ptr_static_cast<SmallNonVirtualBase>( spSmall );
				soft_ptr<int> pintSmall( pSmall, &(pSmall->sn) );

				owning_ptr<Large> pLarge = make_owning<Large>();
				soft_ptr<Large> spLarge(pLarge);
				soft_ptr<LargeVirtualBase> spLargeVirtualBase = soft_ptr_reinterpret_cast<LargeVirtualBase>( spLarge );
				soft_ptr<LargeNonVirtualBase> spLargeNonVirtualBase = soft_ptr_static_cast<LargeNonVirtualBase>( spLarge );
				soft_ptr<int> pintLarge( pLarge, &(pLarge->lm) );

				owning_ptr<Multiple> pMultiple = make_owning<Multiple>();
				soft_ptr<Multiple> spMultiple(pMultiple);
				soft_ptr<LargeVirtualBase> spMultipleViaLarge = soft_ptr_reinterpret_cast<LargeVirtualBase>( spMultiple );
				soft_ptr<SmallVirtualBase> spMultipleViaSmall = soft_ptr_reinterpret_cast<SmallVirtualBase>( spMultiple );
				soft_ptr<int> pintMultiple( pMultiple, &(pMultiple->mm3) );

				EXPECT_THROWS( soft_ptr<int> pintError1( pMultiple, nullptr ) );

				int * anyN = new int;
				EXPECT_THROWS( soft_ptr<int> pintError2( pMultiple, anyN ) );
				delete anyN;
			}
		},

		CASE( "test is-on-stack" )
		{
			SETUP("test is-on-stack")
			{
				int a;
				//EXPECT( nodecpp::platform::is_guaranteed_on_stack( &a ) );
				int* pn = new int;
				class Large { public: int val[0x10000];};
				Large l;
				EXPECT( nodecpp::platform::is_guaranteed_on_stack( &a ) );
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( pn ) );
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &g_int ) );
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &th_int ) );
				//EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &l ) );
			}
		},

		CASE( "test destruction means" )
		{
			EXPECT_NO_THROW( StartupChecker::check() );
		},
	};

	int ret = lest::run( specification, argc, argv );
	return ret;
}
#endif

void test__allocated_ptr_and_ptr_and_data_and_flags()
{
#ifdef NODECPP_X64
	constexpr size_t maxData = 32;
#else
	constexpr size_t maxData = 26;
#endif
	nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<maxData, 1> obj;

	// allocated_ptr_and_ptr_and_data_and_flags::init()
	obj.init();
	assert( obj.get_data() == 0 );
	assert( obj.get_ptr() == 0 );
	assert( obj.get_allocated_ptr() == 0 );
	assert( !obj.has_flag<0>() );

	// allocated_ptr_and_ptr_and_data_and_flags::init(size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.init(data);
		size_t retData = obj.get_data();
		assert( data == retData );

		obj.init(0);
		retData = obj.get_data();
		assert( 0 == retData );

		obj.init(data);
		retData = obj.get_data();
		assert( data == retData );

		assert( obj.get_ptr() == 0 );
		assert( obj.get_allocated_ptr() == 0 );
		assert( !obj.has_flag<0>() );
	}

	obj.init();

	// allocated_ptr_and_ptr_and_data_and_flags::init(void*, void*, size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;

		obj.init(nullptr, nullptr, data);
		size_t retData = obj.get_data();
		assert( data == retData );

		assert( obj.get_ptr() == 0 );
		assert( obj.get_allocated_ptr() == 0 );
		assert( !obj.has_flag<0>() );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=0; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);

		obj.init(ptr, nullptr, 0);
		void* retPtr = obj.get_ptr();
		assert( ptr == retPtr );

		assert( obj.get_allocated_ptr() == 0 );
		assert( !obj.has_flag<0>() );
		assert( obj.get_data() == 0 );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.init(nullptr, ptr, 0);
		void* retPtr = obj.get_allocated_ptr();
		assert( ptr == retPtr );

		assert( obj.get_ptr() == 0 );
		assert( !obj.has_flag<0>() );
		assert( obj.get_data() == 0 );

		obj.init(0);
	}

	// testing setters

	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.set_data(data);
		size_t retData = obj.get_data();
		assert( data == retData );

		obj.set_data(0);
		retData = obj.get_data();
		assert( 0 == retData );

		obj.set_data(data);
		retData = obj.get_data();
		assert( data == retData );

		assert( obj.get_ptr() == 0 );
		assert( obj.get_allocated_ptr() == 0 );
		assert( !obj.has_flag<0>() );
	}

	obj.init();

	assert( !obj.has_flag<0>() );
	obj.set_flag<0>();
	assert( obj.has_flag<0>() );

	assert( obj.get_ptr() == 0 );
	assert( obj.get_allocated_ptr() == 0 );
	assert( obj.get_data() == 0 );

	obj.unset_flag<0>();
	assert( !obj.has_flag<0>() );

	assert( obj.get_ptr() == 0 );
	assert( obj.get_allocated_ptr() == 0 );
	assert( obj.get_data() == 0 );

	for ( size_t i=0; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		assert( ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		assert( 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		assert( ptr == retPtr );

		assert( obj.get_allocated_ptr() == 0 );
		assert( !obj.has_flag<0>() );
		assert( obj.get_data() == 0 );
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_allocated_ptr(ptr);
		void* retPtr = obj.get_allocated_ptr();
		assert( ptr == retPtr );

		obj.set_allocated_ptr(0);
		retPtr = obj.get_allocated_ptr();
		assert( 0 == retPtr );

		obj.set_allocated_ptr(ptr);
		retPtr = obj.get_allocated_ptr();
		assert( ptr == retPtr );
	
		assert( obj.get_ptr() == 0 );
		assert( !obj.has_flag<0>() );
		assert( obj.get_data() == 0 );
	}

}

void test__allocated_ptr_with_mask_and_flags()
{
	constexpr size_t maskSize = 3;
	constexpr size_t maskMax = ((size_t)1<<maskSize)-1;
	nodecpp::platform::allocated_ptr_with_mask_and_flags<maskSize, 1> obj;
	obj.init();

	for ( size_t mask=0; mask<maskMax; ++mask )
	{
		obj.set_mask(mask);
		size_t retData = obj.get_mask();
		assert( mask == retData );

		obj.set_mask(0);
		retData = obj.get_mask();
		assert( 0 == retData );

		obj.set_mask(mask);
		retData = obj.get_mask();
		assert( mask == retData );

		assert( obj.get_ptr() == 0 );
		assert( !obj.has_flag<0>() );
	}

	obj.init();

	assert( !obj.has_flag<0>() );
	obj.set_flag<0>();
	assert( obj.has_flag<0>() );

	assert( obj.get_ptr() == 0 );
	assert( obj.get_mask() == 0 );

	obj.unset_flag<0>();
	assert( !obj.has_flag<0>() );

	assert( obj.get_ptr() == 0 );
	assert( obj.get_mask() == 0 );

	for ( size_t i=3; i<48;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		assert( ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		assert( 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		assert( ptr == retPtr );

		assert( !obj.has_flag<0>() );
		assert( obj.get_mask() == 0 );
	}

}

int main( int argc, char * argv[] )
{
	//test__allocated_ptr_and_ptr_and_data_and_flags();
	//test__allocated_ptr_with_mask_and_flags(); return 0;

	int any = 0;
	printf( "&any = 0x%zx\n", (size_t)(&any) );
	//testNullPtrAccess(); return 0;
	test__allocated_ptr_and_ptr_and_data_and_flags(); //return 0;

#ifdef SAFE_PTR_DEBUG_MODE
	printf( "   ===>> onStackSafePtrCreationCount = %zd, onStackSafePtrDestructionCount = %zd\n", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//assert( onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // SAFE_PTR_DEBUG_MODE
/*	for ( uint64_t n=0; n<8; ++n )
	{
		unsigned long ix;
		uint64_t n1 = ~n;
		uint8_t r = _BitScanForward(&ix, n1);
		assert( ( (1<<ix) & n ) == 0 || ix > 2 );
		printf( "%zd %zd: %zd (%zd)\n", n, n1, ix, (size_t)r );
	}
	return 0;*/
	//int ret = lest::run( specification, argc, argv );
	/**/int ret = testWithLest( argc, argv );
	killAllZombies();

#ifdef SAFE_PTR_DEBUG_MODE
	printf( "   ===>>onStackSafePtrCreationCount = %zd, onStackSafePtrDestructionCount = %zd\n", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	assert( onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // SAFE_PTR_DEBUG_MODE
    //return ret;
	fnStart();
	fnSoftEnd();
	fnOwningEnd();
#ifdef SAFE_PTR_DEBUG_MODE
	printf( "   ===>>onStackSafePtrCreationCount = %zd, onStackSafePtrDestructionCount = %zd\n", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	assert( onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // SAFE_PTR_DEBUG_MODE
	printf( "about to exit main()...\n" );
	return 0;
}