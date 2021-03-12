// RUN: %check_safememory_instrument --fix-only %s %t %p

#include <safememory/safe_ptr.h>

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

	void verifyZombieArgs() {

		//both args may be zombie
		safeFunction(getSt(), getSt());
// CHECK-FIXES: { auto& nodecpp_0 = getSt(); auto& nodecpp_1 = getSt(); safeFunction(nodecpp_0, nodecpp_1); };

		// literal can't zombie
		// nothing to do here
		safeFunction(getSt(), "hello!");
	}
};
