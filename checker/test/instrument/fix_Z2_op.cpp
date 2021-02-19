// RUN: %check_nodecpp_instrument --fix-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_memory/safe_ptr.h>
#include <safe_memory/dezombiefy.h>
#include <safe_types.h>

using namespace safememory;

struct UnsafeType {

	int call(int i) { return i; }
	UnsafeType& operator<<(int) {
		return *this;
	}

};


struct Bad {

	SafeType StMember;
	owning_ptr<SafeType> StPtr;
	owning_ptr<UnsafeType> UPtr;


	Bad() { 
		StPtr = make_owning<SafeType>(); 
		UPtr = make_owning<UnsafeType>();
	}

	int release() {
		StPtr.reset();
		UPtr.reset();
		return 0;
	}

	void otherMethod(int i) {}
	SafeType& getSt() { return *StPtr; }
	UnsafeType& getU() { return *UPtr; }

	void verifyZombieOperator(SafeType& StRef) {

		getSt() << release();
// CHECK-FIXES: { auto& nodecpp_0 = getSt(); auto nodecpp_1 = release(); nodecpp_0 << nodecpp_1; };

		getSt() << 1 << 2 << release();
// CHECK-FIXES: { auto& nodecpp_2 = getSt(); auto& nodecpp_3 = nodecpp_2 << 1; auto& nodecpp_4 = nodecpp_3 << 2; auto nodecpp_5 = release(); nodecpp_4 << nodecpp_5; };
 	}
};
