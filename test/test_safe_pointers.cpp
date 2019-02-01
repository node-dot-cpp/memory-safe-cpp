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

#include <safe_ptr.h>
#include <startup_checks.h>
#include "../3rdparty/lest/include/lest/lest.hpp"
#include "test_nullptr_access.h"

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
thread_local size_t nodecpp::safememory::onStackSafePtrCreationCount; 
thread_local size_t nodecpp::safememory::onStackSafePtrDestructionCount;
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

thread_local void* nodecpp::safememory::thg_stackPtrForMakeOwningCall = 0;

using namespace nodecpp::safememory;

#ifdef NODECPP_USE_IIBMALLOC
using namespace nodecpp::iibmalloc;
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
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
	}
};
static IIBMallocInitializer iibmallocinitializer;
#elif defined NODECPP_USE_NEW_DELETE_ALLOC
thread_local void** zombieList_ = nullptr;
#endif // NODECPP_USE_xxx_ALLOC

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
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *gsp == 17 );
	soft_ptr<int> sp = std::move(gsp); 
}
void fnOwningEnd() { 
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *gop == 17 );
	gsp.reset();
	gop.reset(); 
}

// prereqs for testing soft_this_ptr
class SomethingLarger; //forward declaration
class Something
{
public:
	soft_this_ptr myThis;
	owning_ptr<int> m;
	soft_ptr<SomethingLarger> prtToOwner;
	Something( int k) { m = make_owning<int>(); *m = k; }
	Something(soft_ptr<SomethingLarger> prtToOwner_, int k);
	Something(bool, soft_ptr<SomethingLarger> prtToOwner_, int k);
	void setOwner(soft_ptr<SomethingLarger> prtToOwner_) { prtToOwner = prtToOwner_ ; }
};
class SomethingLarger
{
public:
	soft_this_ptr myThis;
	soft_ptr<Something> softpS;
	owning_ptr<Something> opS;
	SomethingLarger(int k) : opS( std::move( make_owning<Something>( k ) ) ) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS->setOwner( sp );
		softpS = opS;
	}
	SomethingLarger(int k, bool) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS = make_owning<Something>( sp, k );
	}
	SomethingLarger(int k, bool, bool) {
		soft_ptr<SomethingLarger> sp = myThis.getSoftPtr( this );		
		opS = make_owning<Something>( false, sp, k );
	}
	void doBackRegistration( soft_ptr<Something> s ) { softpS = s; }
};
Something::Something(soft_ptr<SomethingLarger> prtToOwner_, int k) {
	prtToOwner = prtToOwner_;
	soft_ptr<Something> sp = myThis.getSoftPtr( this );
	m = make_owning<int>(); 
	*m = k; 
	prtToOwner->doBackRegistration( sp );
}
Something::Something(bool, soft_ptr<SomethingLarger> prtToOwner_, int k) {
	prtToOwner = prtToOwner_;
	soft_ptr<Something> sp = soft_ptr_in_constructor( this );
	m = make_owning<int>(); 
	*m = k; 
	prtToOwner->doBackRegistration( sp );
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
			 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "[1] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				Ptr2PtrWishData n1D, n2D;
				//SECTION( "initializing ptrs-with-data" )
				{
					n1D.init(n1,Ptr2PtrWishData::invalidData);
					n2D.init(n2,Ptr2PtrWishData::invalidData);
				 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "[2] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					EXPECT( n1D.getPtr() == n1 );
					EXPECT( n2D.getPtr() == n2 );
					EXPECT( n1D.getData() == Ptr2PtrWishData::invalidData );
					EXPECT( n2D.getData() == Ptr2PtrWishData::invalidData );
			 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "[3] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				}
				//SECTION( "updating data" )
				{
			 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "[4] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					n1D.updateData(6);
					n2D.updateData(500000);
				 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "[5] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
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
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				//SECTION( "yet updating data" )
				{
					n1D.updateData(500000);
					n2D.updateData(6);
					EXPECT( n1D.getPtr() == n2 );
					EXPECT( n2D.getPtr() == n1 );
					EXPECT( n1D.getData() == 500000 );
					EXPECT( n2D.getData() == 6 );
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
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
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "*s11 = {}, *s12 = {}, *s11 = {}, *s12 = {}", *s11, *s12, *s21, *s22 );
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "before swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
					s21.swap(s12);
					//soft_ptr<int> tmp1 = s21; s21 = s12; s12 = tmp1;
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "after swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
 					EXPECT( *s11 == 6 );
					EXPECT( *s12 == 26 );
					EXPECT( *s21 == 6 );
					EXPECT( *s22 == 26 );
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "before swapping: *s11 = {}", *s11.get() );
					s01.swap(s11);
					//soft_ptr<int> tmp2 = s01; s01 = s11; s11 = tmp2;
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "after swapping: *s01 = {}", *s01.get() );
 					EXPECT( *s01 == 6 );
					soft_ptr<int> s13(p1);
					soft_ptr<int> s14(p1);
					{
						soft_ptr<int> s15(p1);
 						//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "*s15 = {}", *s15.get() );
						EXPECT( *s15 == 6 );
					}
					soft_ptr<int> s15(p1);
					EXPECT( *s15 == 6 );
 					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "*s15 = {}", *s15.get() );
					soft_ptr<int> s16(p1);
					{
						soft_ptr<int> s17(p1);
						EXPECT( *s17 == 6 );
 						//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "*s17 = {}", *s17.get() );
					}
					EXPECT( *p1 == 6 );
					EXPECT( *p2 == 26 );
					//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "*p1 = {}, *p2 = {}", *p1, *p2 );
					owning_ptr<int> p3 = make_owning<int>();
					*p3 = 17;
					s02 = p3;
					EXPECT( *s02 == 17 );

					soft_ptr<int> s30;
					s30 = std::move( s02 );
					EXPECT( *s30 == 17 );

					soft_ptr<void> sv;
//					sv = std::move( s30 );
					sv = soft_ptr_static_cast<int>( s30 );
					EXPECT( sv );

					soft_ptr<int> s31;
					s31 = soft_ptr_static_cast<int>( sv );
					EXPECT( *s31 == 17 );

					/*class [[nodecpp::owning_only]] S{ public: int m; static bool doSmthWithMySoftPtr(soft_ptr<S> s, int k) { return s->m == k; }  bool callSmthWithMySoftPtr(int k) {return doSmthWithMySoftPtr(soft_ptr<S>(this), k); } };
					owning_ptr<S> sS = make_owning<S>();
					sS->m = 17;
					EXPECT( sS->callSmthWithMySoftPtr(sS->m) );*/
					
					struct StrWithSoftPtr { soft_ptr<int> sp; };
					owning_ptr<int> p4 = make_owning<int>();
					owning_ptr<StrWithSoftPtr> p5 = make_owning<StrWithSoftPtr>();
					p5->sp = p4;
					EXPECT( p5->sp );
					p4.reset();
					EXPECT( !(p5->sp) );

					owning_ptr<int> op1 = make_owning<int>();
					owning_ptr<int> op2 = make_owning<int>();
					soft_ptr<int> sp11( op1 );
					soft_ptr<int> sp12( op1 );
					soft_ptr<int> sp21( op2 );
					EXPECT(  sp11 == op1 );
					EXPECT(  op1 == sp11 );
					EXPECT(  sp11 == sp12 );
					EXPECT(  !(sp11 != op1) );
					EXPECT(  !(op1 != sp11) );
					EXPECT(  !(sp11 != sp12) );
					EXPECT(  sp11 != op2 );
					EXPECT(  op2 != sp11 );
					EXPECT(  sp11 != sp21 );
					EXPECT(  !(sp11 == op2) );
					EXPECT(  !(op2 == sp11) );
					EXPECT(  !(sp11 == sp21) );

				}
				//nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "is s14 == NULL (as it shoudl be)? {}", s14 ? "NO" : "YES" );
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
				// NOTE: test below may occasionally failed: 
				//       stack var can be reported as not being such
				//       but heap value must never be reported as guaranteed on stack
				//int a;
				//EXPECT( nodecpp::platform::is_guaranteed_on_stack( &a ) );

				int* pn = new int;
				//class Large { public: int val[0x10000];};
				//Large l;
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( pn ) );
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &g_int ) );
				EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &th_int ) );
				//EXPECT( !nodecpp::platform::is_guaranteed_on_stack( &l ) );
			}
		},

		CASE( "test soft_this_ptr" )
		{
			SETUP("test soft_this_ptr")
			{
				owning_ptr<SomethingLarger> opSL = make_owning<SomethingLarger>( 17 );
				EXPECT( *(opSL->opS->m) == 17 );
				EXPECT( *(opSL->softpS->m) == 17 );
				owning_ptr<SomethingLarger> opSL_1 = make_owning<SomethingLarger>( 27, false );
				EXPECT( *(opSL_1->opS->m) == 27 );
				EXPECT( *(opSL_1->softpS->m) == 27 );
				owning_ptr<SomethingLarger> opSL_2 = make_owning<SomethingLarger>( 37, false, false );
				EXPECT( *(opSL_2->opS->m) == 37 );
				EXPECT( *(opSL_2->softpS->m) == 37 );
			}
		},

		CASE( "test comparison operators" )
		{
			// TODO: extend to other relevant cases
			owning_ptr<uint32_t> op;
			EXPECT( op == nullptr );
			op = make_owning<uint32_t>(17);
			EXPECT( op != nullptr );
			soft_ptr<uint32_t> sp;
			EXPECT( sp == nullptr );
			sp = op;
			EXPECT( sp != nullptr );
			EXPECT( sp == op );
			EXPECT( op == sp );
			EXPECT( op == op );
			EXPECT( sp == sp );
			owning_ptr<uint32_t> op1 = make_owning<uint32_t>(27);
			soft_ptr<uint32_t> sp1 = op1;
			EXPECT( sp != sp1 );
			EXPECT( sp != op1 );
			// ...
		},

		CASE( "test destruction means" )
		{
			EXPECT_NO_THROW( testing::StartupChecker::check() );
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
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	// allocated_ptr_and_ptr_and_data_and_flags::init(size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.init(data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		obj.init(0);
		retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.init(data);
		retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	// allocated_ptr_and_ptr_and_data_and_flags::init(void*, void*, size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;

		obj.init(nullptr, nullptr, data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=0; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);

		obj.init(ptr, nullptr, 0);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.init(nullptr, ptr, 0);
		void* retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

		obj.init(0);
	}

	// testing setters

	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.set_data(data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		obj.set_data(0);
		retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.set_data(data);
		retData = obj.get_data();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	obj.set_flag<0>();
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.has_flag<0>() );

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

	obj.unset_flag<0>();
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

	for ( size_t i=0; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_allocated_ptr(ptr);
		void* retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_allocated_ptr(0);
		retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_allocated_ptr(ptr);
		retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );
	
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
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
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, mask == retData );

		obj.set_mask(0);
		retData = obj.get_mask();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.set_mask(mask);
		retData = obj.get_mask();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, mask == retData );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	obj.set_flag<0>();
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.has_flag<0>() );

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );

	obj.unset_flag<0>();
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );

	for ( size_t i=3; i<48;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );
	}

}

void test_soft_this_ptr()
{
	owning_ptr<SomethingLarger> opSL = make_owning<SomethingLarger>( 17 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->opS->m) == 17 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->softpS->m) == 17 );
	owning_ptr<SomethingLarger> opSL_1 = make_owning<SomethingLarger>( 27 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->opS->m) == 27 );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->softpS->m) == 27 );

	SomethingLarger sl(37);
}

int main( int argc, char * argv[] )
{
	//test_soft_this_ptr(); return 0;
	//test__allocated_ptr_and_ptr_and_data_and_flags();
	//test__allocated_ptr_with_mask_and_flags(); return 0;

	int any = 0;
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "&any = 0x{:x}", (size_t)(&any) );
	//testNullPtrAccess(); return 0;
	test__allocated_ptr_and_ptr_and_data_and_flags(); //return 0;

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "   ===>> onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
/*	for ( uint64_t n=0; n<8; ++n )
	{
		unsigned long ix;
		uint64_t n1 = ~n;
		uint8_t r = _BitScanForward(&ix, n1);
		NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, ( (1<<ix) & n ) == 0 || ix > 2 );
		nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "{} {}: {} ({})", n, n1, ix, (size_t)r );
	}
	return 0;*/
	//int ret = lest::run( specification, argc, argv );
	/**/int ret = testWithLest( argc, argv );
	killAllZombies();

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
    //return ret;
	fnStart();
	fnSoftEnd();
	fnOwningEnd();
#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	NODECPP_ASSERT(nodecpp::safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::log<nodecpp::safememory::module_id, nodecpp::log::LogLevel::info>( "about to exit main()..." );
	return 0;
}