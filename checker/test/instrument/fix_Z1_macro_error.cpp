// RUN: %check_nodecpp_instrument --fix-only --no-silent-mode %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_memory/safe_ptr.h>
#include <safe_memory/dezombiefy.h>
#include <safe_types.h>

#define MACRO_ADD(X, Y) ((X) + (Y))

using namespace safe_memory;
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
