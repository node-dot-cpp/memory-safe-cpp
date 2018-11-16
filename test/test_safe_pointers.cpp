// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdio.h>

#include "../src/safe_ptr.h"

void testPtrsWithData()
{
	int* n1 = new int;
	int* n2 = new int;
	printf( "n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
	Ptr2PtrWishData n1D, n2D;
	n1D.init(n1,Ptr2PtrWishData::invalidData);
	n2D.init(n2,Ptr2PtrWishData::invalidData);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
	n1D.updateData(6);
	n2D.updateData(500000);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
	n1D.updatePtr(n2);
	n2D.updatePtr(n1);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
	n1D.updateData(500000);
	n2D.updateData(6);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
}

void testSafePointers()
{
	soft_ptr<int> s13;
	soft_ptr<int> s14;
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
//		printf( "*n1 = %d, *n2 = %d\n", *n1, *n2 );
		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s21.swap(s12);
 		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s14.swap(s11);
 		printf( "*s14 = %d\n", *s14.get() );
		soft_ptr<int> s13(p1);
		soft_ptr<int> s14(p1);
		{
			soft_ptr<int> s15(p1);
 			printf( "*s15 = %d\n", *s15.get() );
		}
		soft_ptr<int> s15(p1);
 		printf( "*s15 = %d\n", *s15.get() );
		soft_ptr<int> s16(p1);
		{
			soft_ptr<int> s17(p1);
 			printf( "*s17 = %d\n", *s17.get() );
		}
		printf( "*p1 = %d, *p2 = %d\n", *p1, *p2 );
		owning_ptr<int> p3 = make_owning<int>();
		*p3 = 17;
		printf( "*p3 = %d\n", *p3 );
	}
	printf( "is s14 == NULL (as it shoudl be)? %s\n", s14 ? "NO" : "YES" );
}

void testCasts()
{
	class Base { public: virtual ~Base(){} };
	class Derived : public Base { public: virtual ~Derived(){} };
	owning_ptr<Derived> p = make_owning<Derived>();
	soft_ptr<Base> p1 = p;
	soft_ptr<Derived> p2 = soft_ptr_static_cast<Derived>(p1);
	soft_ptr<Derived> p3 = soft_ptr_reinterpret_cast<Derived>(p1);
//	assert( p.get() == p2.get() );
//	assert( p3.get() == p2.get() );
}

int main()
{
	testPtrsWithData();
	printf( "\n" );
	testSafePointers();
	printf( "\n" );
	testCasts();

	return 0;
}

