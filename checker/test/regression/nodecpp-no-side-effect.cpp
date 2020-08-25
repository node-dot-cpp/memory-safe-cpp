// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>
#include <check_at_instantiation.h>

using namespace safe_memory;

void normalFunc();

[[safe_memory::no_side_effect]]
int func(int i, int j) {
    return i + j;   
}

int func2(int, int);

[[safe_memory::no_side_effect]]
int func2(int i, int j) {
// CHECK: :[[@LINE-1]]:5: error: (C3)
    return i + j;   
}

[[safe_memory::no_side_effect]]
int func3();

int func3() {}
// CHECK: :[[@LINE-1]]:5: error: (C3)

struct Basic {};

struct Other {
    Other();
};


[[safe_memory::no_side_effect]]
void badFunc() {
    func(1, 2);//ok
    normalFunc();//bad
// CHECK: :[[@LINE-1]]:5: error: function with no_side_effect attribute can call only other no side effect functions [no-side-effect]

    Basic b;
    Other o;
// CHECK: :[[@LINE-1]]:11: error: function with no_side_effect attribute can call only other no side effect functions [no-side-effect]
}

class SomeClass {
    [[safe_memory::no_side_effect]]
    void badMethod() {
        func(1, 2);//ok
        normalFunc();//bad
// CHECK: :[[@LINE-1]]:9: error: function with no_side_effect attribute can call only other no side effect functions [no-side-effect]
    }

};



class MyClass {
public:
	bool operator==(const MyClass& other) const {
		return false;
	}
};

template<class T>
class EqualTo {
public:
    //for this error we must trigger the actual instantiation of this method
	[[safe_memory::no_side_effect]] bool operator()(const T& l, const T& r) const {
		return l == r;
// CHECK: :[[@LINE-1]]:12: error: function with no_side_effect attribute can call only other no side effect functions
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
	// we must trigger the actual instantiation of the method above
	bool b = mc.isEq();
}




