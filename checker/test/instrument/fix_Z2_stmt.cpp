// RUN: %check_safememory_instrument --fix-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_memory/safe_ptr.h>

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

	 void verifyZombieStmt() {

		int i = getSt().call(release());
// CHECK-FIXES: auto& nodecpp_0 = getSt(); auto nodecpp_1 = release(); int i = nodecpp_0.call(nodecpp_1);

		if(getSt().call(release()) != 0) {}
// CHECK-FIXES: auto& nodecpp_2 = getSt(); auto nodecpp_3 = release(); auto nodecpp_4 = nodecpp_2.call(nodecpp_3); if(nodecpp_4 != 0) {}	

		if(int j = getSt().call(release())) {}
// CHECK-FIXES: auto& nodecpp_5 = getSt(); auto nodecpp_6 = release(); if(int j = nodecpp_5.call(nodecpp_6)) {}


		int j, k = getSt().call(release());
	 }
};
