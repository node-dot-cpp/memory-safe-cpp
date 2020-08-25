// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"
// XFAIL: *

#include <check_at_instantiation.h>

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

	Container<MyClass, BadEqualTo<MyClass>> mc;
	// we must trigger the actual instantiation of the method above
	bool b = mc.isEq();


	Container<MyClass, 
	GoodEqualTo<MyClass>> mc2;
	// we must trigger the actual instantiation of the method above
	bool b2 = mc2.isEq();
}

