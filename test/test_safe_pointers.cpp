// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdint.h>
#include <memory>
#include <stdio.h>

#include "../src/safe_ptr_2.h"

void testPtrsWithData()
{
	int* n1 = new int;
	int* n2 = new int;
	printf( "n1 = 0x%llx, n2 = 0x%llx\n", (uintptr_t)n1, (uintptr_t)n2 );
	Ptr2PtrWishData n1D, n2D;
	n1D.init(n1);
	n2D.init(n2);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
	n1D.updateData(6);
	n1D.updateData(500000);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
	n1D.updatePtr(n2);
	n1D.updatePtr(n1);
	printf( "n1D.ptr = 0x%llx, n1D.data = %zd, n2D.ptr = 0x%llx, n2D.data = %zd\n", (uintptr_t)(n1D.getPtr()), n1D.getData(), (uintptr_t)(n2D.getPtr()), n2D.getData() );
}

void testSafePointers()
{
	SoftPtr<int> s13;
	SoftPtr<int> s14;
	{
		OwningPtr<int> p1 = make_owning<int>();
		*(p1.get()) = 5;
		OwningPtr<int> p2 = make_owning<int>();
		*(p2.get()) = 25;
		SoftPtr<int> s11(p1);
		SoftPtr<int> s12(p1);
		SoftPtr<int> s21(p2);
		SoftPtr<int> s22(p2);
		*s11.get() += 1;
		*s22.get() += 1;
//		printf( "*n1 = %d, *n2 = %d\n", *n1, *n2 );
		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s21.swap(s12);
 		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s14.swap(s11);
 		printf( "*s14 = %d\n", *s14.get() );
	}
	printf( "is s14 == NULL (as it shoudl be)? %s\n", s14 ? "NO" : "YES" );
}

int main()
{
	testPtrsWithData();
	printf( "\n" );
	testSafePointers();

	return 0;
}

