// RUN: %check_nodecpp_instrument --report-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

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

	void verifyZombieThis(SafeType& StRef, UnsafeType& URef) {

		safeFunction(release());//func, no 'this' to zombie

		otherMethod(release());//same 'this' as we, can't zombie

		StMember.call(release()); //Same this as we, can't zombie

		SafeType StVal;
		StVal.call(release());// 'this' can't zombie

		StRef.call(release());//StRef may be deleted, and 'this' will zombie
// CHECK-MESSAGES: :[[@LINE-1]]:9: error: (Z2)
		StPtr->call(release());// StPtr may be deleted, and 'this' will zombie
// CHECK-MESSAGES: :[[@LINE-1]]:10: error: (Z2)
		getSt().call(release());// ref may be deleted, and 'this' will zombie
// CHECK-MESSAGES: :[[@LINE-1]]:11: error: (Z2)

		//same thing on unsafe types is not an issue
		//since we will instrument to dezombiefy 'this'
		// inside UnsafeType::call()
		URef.call(release());
		UPtr->call(release());
		getU().call(release());
	}
};
