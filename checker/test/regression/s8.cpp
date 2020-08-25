// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>

namespace [[safe_memory::memory_unsafe]] name {
	class UnsafeButOk {
		int *P = nullptr;
	};
}

void func() {
	
	int i = 0;
	memset(&i,0,sizeof(i));
// CHECK: :[[@LINE-1]]:2: error: (S8)


	func(); // ok, defined in safe code

	name::UnsafeButOk Ok; // ok, unsafe but [[safe_memory::memory_unsafe]]
}

