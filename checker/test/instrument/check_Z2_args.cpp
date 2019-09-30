// RUN: %check_nodecpp_instrument --report-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_ptr.h>
#include <dezombiefy.h>
#include <safe_types.h>

using namespace nodecpp::safememory;

struct UnsafeType {

	void call(int) { }
};

void unsafeFunction(UnsafeType&, UnsafeType&);

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

	void verifyZombieThis(SafeType& StRef, UnsafeType& URef) {

		SafeType StVal;
		safeFunction(StVal, getSt());//this should be ok, StVal can't zombie

		safeFunction(getSt(), getSt());//both args may be zombie
// CHECK-MESSAGES: :[[@LINE-1]]:3: error: (Z2)
// CHECK-MESSAGES: :[[@LINE-2]]:3: error: (Z2)

		unsafeFunction(getU(), getU());//on unsafe types is not an issue

	}
};
