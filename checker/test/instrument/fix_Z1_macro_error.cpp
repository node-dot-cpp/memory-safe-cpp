// RUN: %check_safememory_instrument --fix-only --no-silent-mode %s %t %p

#include <safememory/safe_ptr.h>

#include <safe_types.h>

#define MACRO_ADD(X, Y) ((X) + (Y))

using namespace safememory;
using namespace nodecpp;


int release() {	return 0; }


struct Bad {

	owning_ptr<SafeType> StPtr;



	Bad() {
		StPtr = make_owning<SafeType>(); 
	}

	// int release() {
	// 	StPtr.reset();
	// 	return 0;
	// }


	void verifyZombie(SafeType& StRef) {

		//safeFunction may be eating a zombie
		while(MACRO_ADD(safeFunction(*StPtr), release()));
//CHECK-MESSAGES: :[[@LINE-1]]:9: error: Op2Call couldn't complete because of MACRO
	}
};
