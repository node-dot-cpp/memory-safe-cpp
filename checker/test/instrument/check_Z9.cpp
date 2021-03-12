// RUN: %check_safememory_instrument --report-only %s %t %p

#include <safememory/safe_ptr.h>

#include <safe_types.h>

using namespace safememory;
using namespace nodecpp;

struct TrivialUnsafeType {

	void call(int) { }
};

struct UnsafeType {

	UnsafeType() {}
	UnsafeType(const UnsafeType&) {}// non trivially copiable
	void call(int) { }
};

struct Bad {

	void verifyZombie(SafeType& StRef) {

		safeTemplate(StRef, TrivialUnsafeType()); //should be ok


		safeTemplate(StRef, UnsafeType()); //should be error
// CHECK-MESSAGES: :[[@LINE-1]]:23: error: (Z9)
	}
};
