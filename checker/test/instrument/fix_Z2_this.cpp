// RUN: %check_safememory_instrument --fix-only %s %t

#include <safememory/safe_ptr.h>

#include <safe_types.h>

using namespace safememory;
using namespace nodecpp;

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

	void verifyZombieThis(SafeType& StRef) {

		//only cases that need fix are here

		//StRef may be deleted, and 'this' will zombie
		StRef.call(release());
// CHECK-FIXES: { auto nodecpp_0 = release(); StRef.call(nodecpp_0); };

		// StPtr may be deleted, and 'this' will zombie
		StPtr->call(release());
// CHECK-FIXES: { auto nodecpp_1 = &*(StPtr); auto nodecpp_2 = release(); nodecpp_1->call(nodecpp_2); };
		
		// ref may be deleted, and 'this' will zombie
		getSt().call(release());
// CHECK-FIXES: { auto& nodecpp_3 = getSt(); auto nodecpp_4 = release(); nodecpp_3.call(nodecpp_4); };

	}
};
