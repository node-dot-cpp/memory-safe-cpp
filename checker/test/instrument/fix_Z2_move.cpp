// RUN: %check_safememory_instrument --fix-only %s %t

#include <safememory/safe_ptr.h>

#include <safe_types.h>

using namespace safememory;

struct UnsafeType {

	int call(int i) { return i; }
	UnsafeType& operator<<(int) {
		return *this;
	}

};

void unsafeByValue(UnsafeType&, UnsafeType);


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

	void verifyZombieMove(SafeType& StRef) {

		safeByValue(getSt(), std::move(StRef));
// CHECK-FIXES: { auto& nodecpp_0 = getSt(); safeByValue(nodecpp_0, std::move(StRef)); };

		safeByValue(getSt(), std::move(getSt()));
// CHECK-FIXES: { auto& nodecpp_1 = getSt(); auto& nodecpp_2 = getSt(); safeByValue(nodecpp_2, std::move(nodecpp_1)); };

		unsafeByValue(getU(), std::move(getU()));
	}
};
