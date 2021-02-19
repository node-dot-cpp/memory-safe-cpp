// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <check_as_user_code.h>

class MyClass {
public:
	bool operator==(const MyClass& other) const {
		return false;
	}
};

template<class T, class Eq>
class Container {
	T t;
	Eq eq;
public:
	bool isEq() const {
		return eq(t, t);
	}
};

void func() {

	Container<MyClass, safememory::BadEqualTo<MyClass>> mc;
	// we must trigger the actual instantiation of the method above
	bool b = mc.isEq();


	Container<MyClass, safememory::GoodEqualTo<MyClass>> mc2;
	// we must trigger the actual instantiation of the method above
	bool b2 = mc2.isEq();
// in this case bad implementation wont report error. Good implementation will.
// error is reported as located in header file, so we check the full location.

// CHECK: check_as_user_code.h:51:18: error: function with no_side_effect attribute	
}

