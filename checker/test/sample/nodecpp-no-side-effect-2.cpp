// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"



//#include <utility>
//#include <nodecpp/awaitable.h>

class MyClass {
public:
	bool operator==(const MyClass& other) const {
		return false;
	}
};

template<class T>
class EqualTo {
public:
	[[nodecpp::no_side_effect]] bool operator()(const T& l, const T& r) const {
		return l == r;
	}
};

template<class T, class Eq = EqualTo<T>>
class Container {
	T t;
	Eq eq;
public:
	bool isEq() const {
		return eq(t, t);
	}
};


void func() {

	Container<MyClass> mc;
	// we must trigger the actual instantiation of the method
	bool b = mc.isEq();
// CHECK: :[[@LINE-1]]:12: error: function with no_side_effect attribute can call only other no side effect functions
}

