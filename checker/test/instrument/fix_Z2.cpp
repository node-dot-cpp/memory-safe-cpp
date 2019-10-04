// RUN: %check_nodecpp_instrument --fix-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_ptr.h>
#include <dezombiefy.h>
#include <safe_types.h>

using namespace nodecpp::safememory;

struct UnsafeType {

	void call(int) { }
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

	void verifyZombieThis(SafeType& StRef) {

		//only cases that need fix are here

		StRef.call(release());//StRef may be deleted, and 'this' will zombie
// CHECK-FIXES: { auto nodecpp_0 = release(); StRef.call(nodecpp_0); };
		StPtr->call(release());// StPtr may be deleted, and 'this' will zombie
// CHECK-FIXES: { auto nodecpp_1 = &*(StPtr); auto nodecpp_2 = release(); nodecpp_1->call(nodecpp_2); };
		getSt().call(release());// ref may be deleted, and 'this' will zombie
// CHECK-FIXES: { auto& nodecpp_3 = getSt(); auto nodecpp_4 = release(); nodecpp_3.call(nodecpp_4); };

	}

	void verifyZombieArgs() {

		//both args may be zombie
		safeFunction(getSt(), getSt());
// CHECK-FIXES: { auto& nodecpp_5 = getSt(); auto& nodecpp_6 = getSt(); safeFunction(nodecpp_5, nodecpp_6); };

	}

};
