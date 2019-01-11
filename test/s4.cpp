// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;

void soft_func(soft_ptr<int> sp) {

}

void owning_func(owning_ptr<int> sp) {
	
}

void func() {
	
	new int;
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S4)

	new int();
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S4)
	new int[5];
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S4)

	void* ptr = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: (S1.3)
	delete ptr;
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S4)

	owning_ptr<int> iop = make_owning<int>(5);

	iop = make_owning<int>(7);

	make_owning<int>(0);
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S4.1)

	soft_ptr<int> sp;
	sp = make_owning<int>(2);
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: (S4.1)

	owning_func(make_owning<int>(2));
	soft_func(make_owning<int>(2));
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: (S4.1)

}

