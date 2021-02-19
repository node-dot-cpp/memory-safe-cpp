// RUN: %check_nodecpp_instrument --report-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers

#include <safe_memory/safe_ptr.h>
#include <safe_memory/dezombiefy.h>
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
