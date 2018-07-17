// TestSafePointers.cpp : Defines the entry point for the console application.
//

#include <stdint.h>
#include <memory>
#include <stdio.h>

#include "../src/safe_ptr.h"

int main()
{
	SoftPtr<int> s13;
	SoftPtr<int> s14;
	{
		int* n1 = new int;
		*n1 = 5;
		int* n2 = new int;
		*n2 = 25;
		OwningPtr<int> p1(n1);
		OwningPtr<int> p2(n2);
		SoftPtr<int> s11(p1);
		SoftPtr<int> s12(p1);
		SoftPtr<int> s21(p2);
		SoftPtr<int> s22(p2);
		*s11.get() += 1;
		*s22.get() += 1;
		printf( "*n1 = %d, *n2 = %d\n", *n1, *n2 );
		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s21.swap(s12);
 		printf( "*s11 = %d, *s12 = %d, *s11 = %d, *s12 = %d\n", *s11.get(), *s12.get(), *s21.get(), *s22.get() );
		s14.swap(s11);
 		printf( "*s14 = %d\n", *s14.get() );
	}
	printf( "*s14 = %d\n", *s14.get() );
	return 0;
}

