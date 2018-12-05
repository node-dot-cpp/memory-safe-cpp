#ifndef TEST_NULLPTR_ACCESS_H
#define TEST_NULLPTR_ACCESS_H

#include "../src/safe_ptr.h"

void testNullPtrAccess()
{
	try {
		owning_ptr<int> optr = make_owning<int>();
		*optr = 17;
		printf( "[1]accessing allocated memory: OK\n" );
		optr.reset();
		*optr = 27;
	}
	catch (...) {
		printf( "[1]Exception caught\n" );
	}

	class Medium { public: uint64_t arr[0x10]; uint64_t n; };
	try {
		owning_ptr<Medium> medium = make_owning<Medium>();
		medium->n = 17;
		printf( "[2]accessing allocated memory: OK\n" );
		medium.reset();
		medium->n = 27;
	}
	catch (...) {
		printf( "[2]Exception caught\n" );
	}

	class Large { public: uint64_t arr[0x1000]; uint64_t n; };
	try {
		owning_ptr<Large> large = make_owning<Large>();
		large->n = 17;
		printf( "[3]accessing allocated memory: OK\n" );
		large.reset();
		large->n = 27;
	}
	catch (...) {
		printf( "[3]Exception caught\n" );
	}
}

#endif // TEST_NULLPTR_ACCESS_H
