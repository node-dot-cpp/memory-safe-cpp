// RUN: %check_nodecpp_instrument --report-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_ptr.h>
#include <dezombiefy.h>
#include <safe_types.h>

using namespace nodecpp::safememory;

struct Bad {

	owning_ptr<SafeType> StPtr;



	Bad() { 
		StPtr = make_owning<SafeType>(); 
	}

	int release() {
		StPtr.reset();
		return 0;
	}


	void verifyZombie(SafeType& StRef) {

		safeFunction(*StPtr) + release();//safeFunction may be eating a zombie
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: (Z1)
	}
};
