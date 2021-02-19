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

//#include <safe_ptr.h>
//#include <safe_ptr_no_checks.h>
#include "../src/startup_checks.h"
#include "../3rdparty/lest/include/lest/lest.hpp"
//#include "test_nullptr_access.h"
#include "dummy_test_objects.h"
#include <safememory/detail/instrument.h>

//template<> struct safememory::safeness_declarator<double> { static constexpr bool is_safe = false; }; // user-defined exclusion
//template<> struct safememory::safeness_declarator<safememory::testing::dummy_objects::StructureWithSoftPtrDeclaredUnsafe> { static constexpr bool is_safe = false; }; // user-defined exclusion

using namespace safememory;
using namespace safememory::testing::dummy_objects;
using safememory::detail::killAllZombies;
using safememory::detail::interceptNewDeleteOperators;
using safememory::detail::doZombieEarlyDetection;

#ifdef NODECPP_USE_IIBMALLOC
using namespace nodecpp::iibmalloc;
class IIBMallocInitializer
{
public:
	IIBMallocInitializer()
	{
		g_AllocManager.initialize();
//		g_AllocManager.enable();
	}
	~IIBMallocInitializer()
	{
//	nodecpp::log::default_log::error( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
	}
};
static IIBMallocInitializer iibmallocinitializer;
#elif defined NODECPP_USE_NEW_DELETE_ALLOC
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
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *gsp == 17 );
	soft_ptr<int> sp = std::move(gsp); 
}
void fnOwningEnd() { 
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *gop == 17 );
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
			 //nodecpp::log::default_log::error( "[1] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				Ptr2PtrWishData n1D, n2D;
				//SECTION( "initializing ptrs-with-data" )
				{
					n1D.init(n1,Ptr2PtrWishData::invalidData);
					n2D.init(n2,Ptr2PtrWishData::invalidData);
				 //nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::default_log::error( "[2] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					EXPECT( n1D.getPtr() == n1 );
					EXPECT( n2D.getPtr() == n2 );
					EXPECT( n1D.getData() == Ptr2PtrWishData::invalidData );
					EXPECT( n2D.getData() == Ptr2PtrWishData::invalidData );
			 //nodecpp::log::default_log::error( "[3] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				}
				//SECTION( "updating data" )
				{
			 //nodecpp::log::default_log::error( "[4] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					n1D.updateData(6);
					n2D.updateData(500000);
				 //nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::default_log::error( "[5] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
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
					//nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				//SECTION( "yet updating data" )
				{
					n1D.updateData(500000);
					n2D.updateData(6);
					EXPECT( n1D.getPtr() == n2 );
					EXPECT( n2D.getPtr() == n1 );
					EXPECT( n1D.getData() == 500000 );
					EXPECT( n2D.getData() == 6 );
					//nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
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
					//nodecpp::log::default_log::error( "*s11 = {}, *s12 = {}, *s11 = {}, *s12 = {}", *s11, *s12, *s21, *s22 );
					//nodecpp::log::default_log::error( "before swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
					s21.swap(s12);
					//soft_ptr<int> tmp1 = s21; s21 = s12; s12 = tmp1;
					//nodecpp::log::default_log::error( "after swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
 					EXPECT( *s11 == 6 );
					EXPECT( *s12 == 26 );
					EXPECT( *s21 == 6 );
					EXPECT( *s22 == 26 );
					//nodecpp::log::default_log::error( "before swapping: *s11 = {}", *s11.get() );
					s01.swap(s11);
					//soft_ptr<int> tmp2 = s01; s01 = s11; s11 = tmp2;
					//nodecpp::log::default_log::error( "after swapping: *s01 = {}", *s01.get() );
 					EXPECT( *s01 == 6 );
					soft_ptr<int> s13(p1);
					soft_ptr<int> s14(p1);
					{
						soft_ptr<int> s15(p1);
 						//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
						EXPECT( *s15 == 6 );
					}
					soft_ptr<int> s15(p1);
					EXPECT( *s15 == 6 );
 					//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
					soft_ptr<int> s16(p1);
					{
						soft_ptr<int> s17(p1);
						EXPECT( *s17 == 6 );
 						//nodecpp::log::default_log::error( "*s17 = {}", *s17.get() );
					}
					EXPECT( *p1 == 6 );
					EXPECT( *p2 == 26 );
					//nodecpp::log::default_log::error( "*p1 = {}, *p2 = {}", *p1, *p2 );
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
					
					owning_ptr<int> p4int = make_owning<int>();
					owning_ptr<StructureWithSoftIntPtr> p5int = make_owning<StructureWithSoftIntPtr>();
					p5int->sp = p4int;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p5int->sp );
					p4int.reset();
					//EXPECT( !(p5->sp) );
					
					owning_ptr<double> p4double = make_owning<double>();
					owning_ptr<StructureWithSoftDoublePtr> p5double = make_owning<StructureWithSoftDoublePtr>();
					soft_ptr<double, owning_ptr<double>::is_safe> x1(p4double);
					soft_ptr<double> x2(p4double);
					EXPECT( x1.is_safe == memory_safety::none );
					EXPECT( x2.is_safe == memory_safety::none );
					soft_ptr<double> y1(p5double, &(p5double->d));
					soft_ptr<double, owning_ptr<StructureWithSoftDoublePtr>::is_safe> y2(p5double, &(p5double->d));
					EXPECT( y1.is_safe == memory_safety::none );
					EXPECT( y2.is_safe == memory_safety::safe );
					p5double->sp = p4double;
					EXPECT( p5double->sp );
					p4double.reset();
					soft_ptr<double> y1copy(y1);
					soft_ptr<double> y2copy(y2);
					
					owning_ptr<double, memory_safety::safe> p4doubleS = make_owning_2<double, memory_safety::safe>();
					soft_ptr<double> x0S(p4doubleS);
					EXPECT( x0S.is_safe == memory_safety::none );
					soft_ptr<double, owning_ptr<double>::is_safe> x1S(p4doubleS);
					EXPECT( x1S.is_safe == memory_safety::none );
					soft_ptr<double, p4doubleS.is_safe> x2S(p4doubleS);
					EXPECT( x2S.is_safe == memory_safety::safe );
					
					owning_ptr<int, memory_safety::none> p4intS = make_owning_2<int, memory_safety::none>();
					soft_ptr<int, memory_safety::none> xn1NS(p4intS);
					EXPECT( xn1NS.is_safe == memory_safety::none );
					soft_ptr<int, p4intS.is_safe> xn2S(p4intS);
					EXPECT( xn2S.is_safe == memory_safety::none );
					
					owning_ptr<int> p14 = make_owning<int>();
					owning_ptr<StructureWithSoftPtrDeclaredUnsafe> p15 = make_owning<StructureWithSoftPtrDeclaredUnsafe>();
					p15->sp = p14;
					EXPECT( p15->sp );
					p14.reset();
					EXPECT( !(p15->sp) );
					
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

					// naked with naked
					nullable_ptr<int> np1( op1 );
					nullable_ptr<int> np2( sp11 );
					nullable_ptr<int> np3( op2 );

					nullable_ptr<void> np1v( op1 );
					nullable_ptr<void> np2v( sp11 );
					nullable_ptr<void> np3v( op2 );

					EXPECT(  np1 == np2 );
					EXPECT(  np1 != np3 );
					EXPECT(  !(np1 != np2) );
					EXPECT(  !(np1 == np3) );

					EXPECT(  np1 == np2v );
					EXPECT(  np1v != np3 );
					EXPECT(  !(np1 != np2v) );
					EXPECT(  !(np1v == np3) );
				}
				//nodecpp::log::default_log::error( "is s14 == NULL (as it shoudl be)? {}", s14 ? "NO" : "YES" );
				//EXPECT( !s01 );
				//EXPECT( !s02 );
			}
			killAllZombies();
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
			killAllZombies();
		},

		CASE( "basic nullable pointer test" )
		{
			SETUP("basic nullable pointer test")
			{
				int i = 5;
				nullable_ptr<int> vnp(&i);
				int* pi = nullable_cast( vnp );
				nullable_ptr<int> vnp2;
				vnp2 = nullable_cast( &i );
				EXPECT( vnp == vnp2 ); // enabled by default

				vnp2 = nullptr;
				int* pint;
//				pint = nullable_cast(vnp2);
				EXPECT_THROWS( pint = nullable_cast(vnp2) );

				testing::dummy_objects::LargeDerived* pl;
				nullable_ptr<testing::dummy_objects::LargeDerived> npl;
				EXPECT_THROWS( pl = nullable_cast(npl) );
			}
			killAllZombies();
		},

		CASE( "Large objects with large alignment test" )
		{
			SETUP("Large objects with large alignment test")
			{
				using LargeAlignedT = LargeObjectWithControllableAlignment<0x1000, 5>;
				EXPECT( sizeof(LargeAlignedT) >= 0x1000 );
				LargeAlignedT* ptr1 = new LargeAlignedT;
				owning_ptr<LargeAlignedT>p4klarge128aligned = make_owning<LargeAlignedT>();
				EXPECT( (( (uintptr_t)(ptr1) ) & ((1<<5)-1)) == 0 );
				if ( ptr1 ) delete ptr1;

				using ExLargeAlignedT = LargeObjectWithControllableAlignment<0x10000, 5>;
				EXPECT( sizeof(ExLargeAlignedT) >= 0x10000 );
				ExLargeAlignedT* ptr2 = new ExLargeAlignedT;
				owning_ptr<ExLargeAlignedT>p64klarge128aligned = make_owning<ExLargeAlignedT>();
				EXPECT( (( (uintptr_t)(ptr2) ) & ((1<<5)-1)) == 0 );
				if ( ptr1 ) delete ptr2;
			}
			killAllZombies();
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
			killAllZombies();
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
			killAllZombies();
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
			killAllZombies();
		},

		CASE( "test comparison operators" )
		{
			SETUP("comparison operators")
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
			}
			killAllZombies();
		},

		CASE( "test destruction means" )
		{
			SETUP("destruction means")
			{
				EXPECT_NO_THROW( testing::StartupChecker::checkBasics() );
				EXPECT_NO_THROW( testing::StartupChecker::checkSafePointers() );
			}
			killAllZombies();
		},

		CASE( "test non-safe pointers" )
		{
			SETUP("non-safe pointers")
			{
				// TODO: extend to other relevant cases
				owning_ptr<double> op;
				EXPECT( op == nullptr );
				op = make_owning<double>(17);
				EXPECT( op != nullptr );
				soft_ptr<double> sp;
				EXPECT( sp == nullptr );
				sp = op;
				EXPECT( sp != nullptr );
				EXPECT( sp == op );
				EXPECT( op == sp );
				EXPECT( op == op );
				EXPECT( sp == sp );
				owning_ptr<double> op1 = make_owning<double>(27);
				soft_ptr<double> sp1 = op1;
				EXPECT( sp != sp1 );
				EXPECT( sp != op1 );/**/
				// ...
			}
			killAllZombies();
		},

		CASE( "test early zombie detaction" )
		{
			SETUP("early zombie detaction")
			{
				owning_ptr<StructureWithSoftIntPtr> opS = make_owning<StructureWithSoftIntPtr>();
				auto ptr = &(opS->n);
				EXPECT_NO_THROW( *(detail::dezombiefy(ptr)) = 17 );
				opS = nullptr;
				EXPECT_THROWS( *(detail::dezombiefy(ptr)) = 27 );
			}
		},

		CASE( "massive referencing" )
		{
			SETUP("massive referencing")
			{
				const size_t maxPtrs = 0x100000;
				soft_ptr<int>* sptrs = new soft_ptr<int>[maxPtrs];
				owning_ptr<int> op = make_owning<int>();
				for ( size_t i=0; i<maxPtrs; ++i )
					sptrs[i] = op;
				op = nullptr;
				for ( size_t i=0; i<maxPtrs; ++i )
					EXPECT( sptrs[i] == nullptr );
				delete [] sptrs;
			}
		},

		CASE( "soft ptrs to me are valid in dtor" )
		{
			SETUP("soft ptrs to me are valid in dtor")
			{
				owning_ptr<StructWithSoftPtr> op1 = make_owning<StructWithSoftPtr>(1);
				owning_ptr<StructWithDtorRequiringValidSoftPtrsToItself> op2 = make_owning<StructWithDtorRequiringValidSoftPtrsToItself>(0, op1);
				op2 = nullptr;
				EXPECT( op1->dummy == 0 );
			}
		},
	};

	int ret = lest::run( specification, argc, argv );
	return ret;
}
#else
int testWithLest( int argc, char * argv[] )
{
	//const lest::test specification[] =
	{
		//CASE( "testing pointers-with-data" )
		{
			//SETUP("testing pointers-with-data")
			{
	#if 0 // TODO: rework for new data structures
				int* n1 = new int;
				int* n2 = new int;
			 //nodecpp::log::default_log::error( "[1] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				Ptr2PtrWishData n1D, n2D;
				////SECTION( "initializing ptrs-with-data" )
				{
					n1D.init(n1,Ptr2PtrWishData::invalidData);
					n2D.init(n2,Ptr2PtrWishData::invalidData);
				 //nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::default_log::error( "[2] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getPtr() == n1 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getPtr() == n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getData() == Ptr2PtrWishData::invalidData );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getData() == Ptr2PtrWishData::invalidData );
			 //nodecpp::log::default_log::error( "[3] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
				}
				////SECTION( "updating data" )
				{
			 //nodecpp::log::default_log::error( "[4] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					n1D.updateData(6);
					n2D.updateData(500000);
				 //nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
			 //nodecpp::log::default_log::error( "[5] n1 = 0x{:x}, n2 = 0x{:x}", (uintptr_t)n1, (uintptr_t)n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getPtr() == n1 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getPtr() == n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getData() == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getData() == 500000 );
				}
				////SECTION( "updating pointers" )
				{
					n1D.updatePtr(n2);
					n2D.updatePtr(n1);
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getPtr() == n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getPtr() == n1 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getData() == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getData() == 500000 );
					//nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				////SECTION( "yet updating data" )
				{
					n1D.updateData(500000);
					n2D.updateData(6);
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getPtr() == n2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getPtr() == n1 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n1D.getData() == 500000 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, n2D.getData() == 6 );
					//nodecpp::log::default_log::error( "n1D.ptr = 0x{:x}, n1D.data = {}, n2D.ptr = 0x{:x}, n2D.data = {}", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
				}
				delete n1;
				delete n2;
	#endif // 0
			}
		}

		//CASE( "basic safe pointer test" )
		{
			//EXPECT_NO_THROW( basicSafePointerTest() );
			//SETUP("basic safe pointer test")
			{
				soft_ptr<int> s01;
				soft_ptr<int> s02;
				//SECTION( "narrowing scope [1]" )
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

					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s11 == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s12 == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s21 == 26 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s22 == 26 );
					//nodecpp::log::default_log::error( "*s11 = {}, *s12 = {}, *s11 = {}, *s12 = {}", *s11, *s12, *s21, *s22 );
					//nodecpp::log::default_log::error( "before swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
					s21.swap(s12);
					//soft_ptr<int> tmp1 = s21; s21 = s12; s12 = tmp1;
					//nodecpp::log::default_log::error( "after swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
 					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s11 == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s12 == 26 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s21 == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s22 == 26 );
					//nodecpp::log::default_log::error( "before swapping: *s11 = {}", *s11.get() );
					s01.swap(s11);
					//soft_ptr<int> tmp2 = s01; s01 = s11; s11 = tmp2;
					//nodecpp::log::default_log::error( "after swapping: *s01 = {}", *s01.get() );
 					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s01 == 6 );
					soft_ptr<int> s13(p1);
					soft_ptr<int> s14(p1);
					{
						soft_ptr<int> s15(p1);
 						//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
						NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s15 == 6 );
					}
					soft_ptr<int> s15(p1);
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s15 == 6 );
 					//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
					soft_ptr<int> s16(p1);
					{
						soft_ptr<int> s17(p1);
						NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s17 == 6 );
 						//nodecpp::log::default_log::error( "*s17 = {}", *s17.get() );
					}
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *p1 == 6 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *p2 == 26 );
					//nodecpp::log::default_log::error( "*p1 = {}, *p2 = {}", *p1, *p2 );
					owning_ptr<int> p3 = make_owning<int>();
					*p3 = 17;
					s02 = p3;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s02 == 17 );

					soft_ptr<int> s30;
					s30 = std::move( s02 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s30 == 17 );

					soft_ptr<void> sv;
//					sv = std::move( s30 );
					sv = soft_ptr_static_cast<int>( s30 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sv );

					soft_ptr<int> s31;
					s31 = soft_ptr_static_cast<int>( sv );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s31 == 17 );

					/*class [[nodecpp::owning_only]] S{ public: int m; static bool doSmthWithMySoftPtr(soft_ptr<S> s, int k) { return s->m == k; }  bool callSmthWithMySoftPtr(int k) {return doSmthWithMySoftPtr(soft_ptr<S>(this), k); } };
					owning_ptr<S> sS = make_owning<S>();
					sS->m = 17;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sS->callSmthWithMySoftPtr(sS->m) );*/
					
					owning_ptr<int> p4int = make_owning<int>();
					owning_ptr<StructureWithSoftIntPtr> p5int = make_owning<StructureWithSoftIntPtr>();
					p5int->sp = p4int;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p5int->sp );
					p4int.reset();
					//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !(p5->sp) );
					
					owning_ptr<double> p4double = make_owning<double>();
					owning_ptr<StructureWithSoftDoublePtr> p5double = make_owning<StructureWithSoftDoublePtr>();
					soft_ptr<double, owning_ptr<double>::is_safe> x(p4double);
					p5double->sp = p4double;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p5double->sp );
					p4double.reset();
					
					owning_ptr<int> p14 = make_owning<int>();
					owning_ptr<StructureWithSoftPtrDeclaredUnsafe> p15 = make_owning<StructureWithSoftPtrDeclaredUnsafe>();
					p15->sp = p14;
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p15->sp );
					p14.reset();
					//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !(p5->sp) );

					owning_ptr<int> op1 = make_owning<int>();
					owning_ptr<int> op2 = make_owning<int>();
					soft_ptr<int> sp11( op1 );
					soft_ptr<int> sp12( op1 );
					soft_ptr<int> sp21( op2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 == op1 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op1 == sp11 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 == sp12 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 != op1) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(op1 != sp11) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 != sp12) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 != op2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op2 != sp11 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 != sp21 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 == op2) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(op2 == sp11) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 == sp21) );

					// naked with naked
					nullable_ptr<int> np1( op1 );
					nullable_ptr<int> np2( sp11 );
					nullable_ptr<int> np3( op2 );

					nullable_ptr<void> np1v( op1 );
					nullable_ptr<void> np2v( sp11 );
					nullable_ptr<void> np3v( op2 );

					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  np1 == np2 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  np1 != np3 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(np1 != np2) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(np1 == np3) );

					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  np1 == np2v );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  np1v != np3 );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(np1 != np2v) );
					NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(np1v == np3) );
				}
				//nodecpp::log::default_log::error( "is s14 == NULL (as it shoudl be)? {}", s14 ? "NO" : "YES" );
				//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !s01 );
				//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !s02 );
			}
		}

		//CASE( "basic safe pointer test" )
		{
			//SETUP("basic safe pointer test")
			{
				class Base { public: int n; virtual ~Base(){} };
				class Derived : public Base { public: virtual ~Derived(){} };
				owning_ptr<Derived> p = make_owning<Derived>();
				p->n = 11;
				soft_ptr<Base> p1 = p;
				soft_ptr<Derived> p2 = soft_ptr_static_cast<Derived>(p1);
				soft_ptr<Derived> p3 = soft_ptr_reinterpret_cast<Derived>(p1);
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p2->n == 11 );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p3->n == 11 );
			}
		}

		//CASE( "test Pointers-To-Members" )
		{
			//SETUP("basic safe pointer test")
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

				{ bool ok = true; try{ soft_ptr<int> pintError1( pMultiple, nullptr ); ok = false; } catch ( ... ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ok ); } }

				int * anyN = new int;
				//EXPECT_THROWS( soft_ptr<int> pintError2( pMultiple, anyN ) );
				{ bool ok = true; try{ soft_ptr<int> pintError2( pMultiple, anyN ); ok = false; } catch ( ... ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ok ); } }
				delete anyN;
			}
		}

		//CASE( "test is-on-stack" )
		{
			//SETUP("test is-on-stack")
			{
				// NOTE: test below may occasionally failed: 
				//       stack var can be reported as not being such
				//       but heap value must never be reported as guaranteed on stack
				//int a;
				//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, nodecpp::platform::is_guaranteed_on_stack( &a ) );

				int* pn = new int;
				//class Large { public: int val[0x10000];};
				//Large l;
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !nodecpp::platform::is_guaranteed_on_stack( pn ) );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !nodecpp::platform::is_guaranteed_on_stack( &g_int ) );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !nodecpp::platform::is_guaranteed_on_stack( &th_int ) );
				//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !nodecpp::platform::is_guaranteed_on_stack( &l ) );
			}
		}

		//CASE( "test soft_this_ptr" )
		{
			//SETUP("test soft_this_ptr")
			{
				owning_ptr<SomethingLarger> opSL = make_owning<SomethingLarger>( 17 );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->opS->m) == 17 );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->softpS->m) == 17 );
				owning_ptr<SomethingLarger> opSL_1 = make_owning<SomethingLarger>( 27, false );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->opS->m) == 27 );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->softpS->m) == 27 );
				owning_ptr<SomethingLarger> opSL_2 = make_owning<SomethingLarger>( 37, false, false );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_2->opS->m) == 37 );
				NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_2->softpS->m) == 37 );
			}
		}

		//CASE( "test comparison operators" )
		{
			// TODO: extend to other relevant cases
			owning_ptr<uint32_t> op;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == nullptr );
			op = make_owning<uint32_t>(17);
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op != nullptr );
			soft_ptr<uint32_t> sp;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == nullptr );
			sp = op;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != nullptr );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == op );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == sp );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == op );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == sp );
			owning_ptr<uint32_t> op1 = make_owning<uint32_t>(27);
			soft_ptr<uint32_t> sp1 = op1;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != sp1 );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != op1 );
			// ...
		}

		//CASE( "test destruction means" )
		{
			{ bool ok = false; try{ testing::StartupChecker::checkBasics(); ok = true; } catch ( ... ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ok ); } }
			{ bool ok = false; try{testing::StartupChecker::checkSafePointers(); ok = true; } catch ( ... ) { NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ok ); } }
			//EXPECT_NO_THROW( testing::StartupChecker::checkBasics() );
			//EXPECT_NO_THROW( testing::StartupChecker::checkSafePointers() );
		}

		//CASE( "test non-safe pointers" )
		{
			// TODO: extend to other relevant cases
			owning_ptr<double> op;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == nullptr );
			op = make_owning<double>(17);
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op != nullptr );
			soft_ptr<double> sp;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == nullptr );
			sp = op;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != nullptr );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == op );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == sp );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, op == op );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp == sp );
			owning_ptr<double> op1 = make_owning<double>(27);
			soft_ptr<double> sp1 = op1;
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != sp1 );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sp != op1 );/**/
			// ...
		}
	};

	//int ret = lest::run( specification, argc, argv );
	//return ret;
	return 0;
}

#endif

void test__allocated_ptr_and_ptr_and_data_and_flags()
{
#ifdef NODECPP_X64
	constexpr size_t maxData = 32;
#else
	constexpr size_t maxData = 26;
#endif
	nodecpp::platform::allocated_ptr_and_ptr_and_data_and_flags<3,maxData, 1> obj;

	// allocated_ptr_and_ptr_and_data_and_flags::init()
	obj.init();
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	// allocated_ptr_and_ptr_and_data_and_flags::init(size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.init(data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		obj.init(0);
		retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.init(data);
		retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	// allocated_ptr_and_ptr_and_data_and_flags::init(void*, void*, size_t)
	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;

		obj.init(nullptr, nullptr, data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=4; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);

		obj.init(ptr, nullptr, 0);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

		obj.init(0);
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.init(nullptr, ptr, 0);
		void* retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

		obj.init(0);
	}

	// testing setters

	for ( size_t i=0; i<maxData;++i )
	{
		size_t data = (size_t)1 << i;
		obj.set_data(data);
		size_t retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		obj.set_data(0);
		retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.set_data(data);
		retData = obj.get_data();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, data == retData );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	obj.set_flag<0>();
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.has_flag<0>() );

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

	obj.unset_flag<0>();
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );

	for ( size_t i=4; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_allocated_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
	}

	obj.init();

	for ( size_t i=3; i<=nodecpp_memory_size_bits;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_allocated_ptr(ptr);
		void* retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_allocated_ptr(0);
		retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_allocated_ptr(ptr);
		retPtr = obj.get_allocated_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );
	
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_data() == 0 );
	}

}

void test__allocated_ptr_with_mask_and_flags()
{
	constexpr size_t maskSize = 3;
	constexpr size_t maskMax = ((size_t)1<<maskSize)-1;
	nodecpp::platform::allocated_ptr_with_mask_and_flags<3,maskSize, 1> obj;
	obj.init();

	for ( size_t mask=0; mask<maskMax; ++mask )
	{
		obj.set_mask(mask);
		size_t retData = obj.get_mask();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, mask == retData );

		obj.set_mask(0);
		retData = obj.get_mask();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retData );

		obj.set_mask(mask);
		retData = obj.get_mask();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, mask == retData );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	}

	obj.init();

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
	obj.set_flag<0>();
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.has_flag<0>() );

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );

	obj.unset_flag<0>();
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );

	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_ptr() == 0 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );

	for ( size_t i=3; i<48;++i )
	{
		void* ptr = (void*)((size_t)1 << i);
		obj.set_ptr(ptr);
		void* retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		obj.set_ptr(0);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, 0 == retPtr );

		obj.set_ptr(ptr);
		retPtr = obj.get_ptr();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ptr == retPtr );

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !obj.has_flag<0>() );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, obj.get_mask() == 0 );
	}

}

void test_soft_this_ptr()
{
	owning_ptr<SomethingLarger> opSL = make_owning<SomethingLarger>( 17 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->opS->m) == 17 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL->softpS->m) == 17 );
	owning_ptr<SomethingLarger> opSL_1 = make_owning<SomethingLarger>( 27 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->opS->m) == 27 );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *(opSL_1->softpS->m) == 27 );

	SomethingLarger sl(37);
}

void test_zombie_objects()
{
}

void temptest()
{
	soft_ptr<int> s01;
	soft_ptr<int> s02;
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

		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s11 == 6 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s12 == 6 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s21 == 26 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s22 == 26 );
		//nodecpp::log::default_log::error( "*s11 = {}, *s12 = {}, *s11 = {}, *s12 = {}", *s11, *s12, *s21, *s22 );
		//nodecpp::log::default_log::error( "before swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
		s21.swap(s12);
		//soft_ptr<int> tmp1 = s21; s21 = s12; s12 = tmp1;
		//nodecpp::log::default_log::error( "after swapping: *s21 = {}, *s12 = {}", *s21, *s12 );
 		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s11 == 6 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s12 == 26 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s21 == 6 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s22 == 26 );
		//nodecpp::log::default_log::error( "before swapping: *s11 = {}", *s11.get() );
		s01.swap(s11);
		//soft_ptr<int> tmp2 = s01; s01 = s11; s11 = tmp2;
		//nodecpp::log::default_log::error( "after swapping: *s01 = {}", *s01.get() );
 		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s01 == 6 );
		soft_ptr<int> s13(p1);
		soft_ptr<int> s14(p1);
		{
			soft_ptr<int> s15(p1);
 			//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s15 == 6 );
		}
		soft_ptr<int> s15(p1);
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s15 == 6 );
 		//nodecpp::log::default_log::error( "*s15 = {}", *s15.get() );
		soft_ptr<int> s16(p1);
		{
			soft_ptr<int> s17(p1);
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s17 == 6 );
 			//nodecpp::log::default_log::error( "*s17 = {}", *s17.get() );
		}
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *p1 == 6 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *p2 == 26 );
		//nodecpp::log::default_log::error( "*p1 = {}, *p2 = {}", *p1, *p2 );
		owning_ptr<int> p3 = make_owning<int>();
		*p3 = 17;
		s02 = p3;
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s02 == 17 );

		soft_ptr<int> s30;
		s30 = std::move( s02 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s30 == 17 );

		/*soft_ptr<void> sv;
//					sv = std::move( s30 );
		sv = soft_ptr_static_cast<int>( s30 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sv );

		soft_ptr<int> s31;
		s31 = soft_ptr_static_cast<int>( sv );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, *s31 == 17 );*/

		/*class [[nodecpp::owning_only]] S{ public: int m; static bool doSmthWithMySoftPtr(soft_ptr<S> s, int k) { return s->m == k; }  bool callSmthWithMySoftPtr(int k) {return doSmthWithMySoftPtr(soft_ptr<S>(this), k); } };
		owning_ptr<S> sS = make_owning<S>();
		sS->m = 17;
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, sS->callSmthWithMySoftPtr(sS->m) );*/
					
		struct StrWithSoftPtr { soft_ptr<int> sp; };
		owning_ptr<int> p4 = make_owning<int>();
		owning_ptr<StrWithSoftPtr> p5 = make_owning<StrWithSoftPtr>();
		p5->sp = p4;
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, p5->sp );
		p4.reset();
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !(p5->sp) );

		owning_ptr<int> op1 = make_owning<int>();
		owning_ptr<int> op2 = make_owning<int>();
		soft_ptr<int> sp11( op1 );
		soft_ptr<int> sp12( op1 );
		soft_ptr<int> sp21( op2 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 == op1 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op1 == sp11 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 == sp12 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 != op1) );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(op1 != sp11) );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 != sp12) );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 != op2 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op2 != sp11 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp11 != sp21 );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 == op2) );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(op2 == sp11) );
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  !(sp11 == sp21) );

	}
	//nodecpp::log::default_log::error( "is s14 == NULL (as it shoudl be)? {}", s14 ? "NO" : "YES" );
	//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !s01 );
	//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, !s02 );
}

// void testSoftPtrsWithZeroOffset()
// {
// 	owning_ptr<int> op = make_owning<int>(17);
// 	lib_helpers::soft_ptr_with_zero_offset<int> spz1( op );
// 	lib_helpers::soft_ptr_with_zero_offset<int> spz2( spz1 );
// 	soft_ptr<int> sp1 = spz2.get();
// 	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op == sp1 );
// 	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp1 == spz1 );
// }

// void testOwningPtrWithManDel()
// {
// 	lib_helpers::owning_ptr_with_manual_delete<int> op = lib_helpers::make_owning_with_manual_delete<int>(17);
// 	lib_helpers::soft_ptr_with_zero_offset<int> spz1( op );
// 	lib_helpers::soft_ptr_with_zero_offset<int> spz2( spz1 );
// 	soft_ptr<int> sp1 = spz2.get();
// 	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op == sp1 );
// 	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  sp1 == spz1 );

// 	soft_ptr<int> sp2( op );
// 	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical,  op == sp2 );
// }

void testStackInfoAndptrLifecycle()
{
	soft_ptr<int>* psp = new soft_ptr<int>; // explicitly non-stack
	{
		owning_ptr<int> op = make_owning<int>( 3 );
		*psp = op;
	}
	printf( "we should not be here with value %d\n", *(*psp) );
}

int main( int argc, char * argv[] )
{
	nodecpp::log::Log log;
	log.level = nodecpp::log::LogLevel::info;
	log.add( stdout );
	nodecpp::logging_impl::currentLog = &log;

	interceptNewDeleteOperators( true );

#ifndef NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, doZombieEarlyDetection( true ) ); // enabled by default
#endif // NODECPP_DISABLE_ZOMBIE_ACCESS_EARLY_DETECTION

	// testSoftPtrsWithZeroOffset();
	// testOwningPtrWithManDel();

	{
		owning_ptr<const int> opci = make_owning<const int>(3);
		soft_ptr<const int> si = opci;
		int k = *si;
		owning_ptr<int> opi = make_owning<int>(33);
		si = opi;
		const int* pcint = &k;
		nullable_ptr<const int> npcint = pcint;
		nullable_ptr<int> npint = &k;
		npcint = &k;
		npcint = npint;
		//*opci = 4;
		/*owning_ptr<int> opci = make_owning<int>(3);
		soft_ptr<const int> si = opci;
		int k = *si;
		owning_ptr<int> opi = make_owning<int>(33);
		si = opi;
		*opci = 4;*/
	}

//temptest(); return 0;
	//test_soft_this_ptr(); return 0;
	//test__allocated_ptr_and_ptr_and_data_and_flags();
	//test__allocated_ptr_with_mask_and_flags(); return 0;

	int any = 0;
	nodecpp::log::default_log::error( "&any = 0x{:x}", (size_t)(&any) );
	//testNullPtrAccess(); return 0;
	test__allocated_ptr_and_ptr_and_data_and_flags(); //return 0;

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::default_log::error( "   ===>> onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	//NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
/*	for ( uint64_t n=0; n<8; ++n )
	{
		unsigned long ix;
		uint64_t n1 = ~n;
		uint8_t r = _BitScanForward(&ix, n1);
		NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, ( (1<<ix) & n ) == 0 || ix > 2 );
		nodecpp::log::default_log::error( "{} {}: {} ({})", n, n1, ix, (size_t)r );
	}
	return 0;*/
	//int ret = lest::run( specification, argc, argv );
	/**/int ret = testWithLest( argc, argv );
	killAllZombies();

#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::default_log::error( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
    //return ret;
	fnStart();
	fnSoftEnd();
	fnOwningEnd();
#ifdef NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING
	nodecpp::log::default_log::error( "   ===>>onStackSafePtrCreationCount = {}, onStackSafePtrDestructionCount = {}", onStackSafePtrCreationCount, onStackSafePtrDestructionCount );
	NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::critical, onStackSafePtrCreationCount == onStackSafePtrDestructionCount );
#endif // NODECPP_ENABLE_ONSTACK_SOFTPTR_COUNTING

	try { testStackInfoAndptrLifecycle(); }
	catch (nodecpp::error::error e)
	{
		e.log(log, nodecpp::log::LogLevel::fatal);
	}
	catch (...)
	{
		nodecpp::log::default_log::fatal("Unknown error happened. About to exit...");
		return 0;
	}
	nodecpp::log::default_log::error( "about to exit main()..." );

	killAllZombies();
	interceptNewDeleteOperators( false );

	nodecpp::log::default_log::fatal( "about to exit..." );

	return 0;
}