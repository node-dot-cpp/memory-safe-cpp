// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;

void soft_func(soft_ptr<int> sp) {

}

void owning_func(owning_ptr<int> sp) {
	
}

void func() {
	
	new int;
// CHECK: :[[@LINE-1]]:2: error: (S4)

	new int();
// CHECK: :[[@LINE-1]]:2: error: (S4)
	new int[5];
// CHECK: :[[@LINE-1]]:2: error: (S4)

	void* ptr = nullptr;
// CHECK: :[[@LINE-1]]:14: error: (S1.2)
	delete ptr;
// CHECK: :[[@LINE-1]]:2: error: (S4)

	owning_ptr<int> iop = make_owning<int>(5);

	iop = make_owning<int>(7);

	make_owning<int>(0);
// CHECK: :[[@LINE-1]]:2: error: (S4.1)

	soft_ptr<int> sp;
	sp = make_owning<int>(2);
// CHECK: :[[@LINE-1]]:7: error: (S4.1)

	owning_func(make_owning<int>(2));
	soft_func(make_owning<int>(2));
// CHECK: :[[@LINE-1]]:12: error: (S4.1)

}

