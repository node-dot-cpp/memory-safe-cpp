// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#include "../src/safe_ptr.h"
#include "../3rdparty/lest/include/lest/lest.hpp"

const lest::test specification[] =
{
	CASE( "testing pointers-with-data" )
	{
		SETUP("testing pointers-with-data")
		{
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
 				EXPECT( *s11 == 6 );
				EXPECT( *s12 == 26 );
				EXPECT( *s21 == 6 );
				EXPECT( *s22 == 26 );
				//printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
				s01.swap(s11);
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
			EXPECT( !s01 );
			EXPECT( !s02 );
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
			class SmallNonVirtualBase { public: int n; int m;};
			class SmallVirtualBase : public SmallNonVirtualBase { public: int n1; int m1; virtual ~SmallVirtualBase() {}};
			class Small : public SmallVirtualBase { public: int n2; int m2;};

			class LargeNonVirtualBase { public: int n[0x10000]; int m;};
			class LargeVirtualBase : public LargeNonVirtualBase { public: int n1; int m1[0x10000]; virtual ~LargeVirtualBase() {}};
			class Large : public LargeVirtualBase { public: int n2; int m2;};

			owning_ptr<Small> pSmall = make_owning<Small>();
			soft_ptr<Small> spSmall(pSmall);
			soft_ptr<SmallVirtualBase> spSmallVirtualBase = soft_ptr_reinterpret_cast<SmallVirtualBase>( spSmall );
			soft_ptr<SmallNonVirtualBase> spSmallNonVirtualBase = soft_ptr_static_cast<SmallNonVirtualBase>( spSmall );
			soft_ptr<int> pintSmall( pSmall, &(pSmall->n) );

			owning_ptr<Large> pLarge = make_owning<Large>();
			soft_ptr<Large> spLarge(pLarge);
			soft_ptr<LargeVirtualBase> spLargeVirtualBase = soft_ptr_reinterpret_cast<LargeVirtualBase>( spLarge );
			soft_ptr<LargeNonVirtualBase> spLargeNonVirtualBase = soft_ptr_static_cast<LargeNonVirtualBase>( spLarge );
			soft_ptr<int> pintLarge( pLarge, &(pLarge->m) );
		}
	},

};

int main( int argc, char * argv[] )
{
    return lest::run( specification, argc, argv );
}