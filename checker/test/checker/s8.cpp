// RUN: nodecpp-checker --safe-library-db=%p/s8.json %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <safe_memory/safe_ptr.h>
#include <server.h>

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

	//ok, allowed at 's8.json'
	safe_memory::owning_ptr<nodecpp::Socket> optr = safe_memory::make_owning<nodecpp::Socket>();

	name::UnsafeButOk Ok; // ok, unsafe but [[safe_memory::memory_unsafe]]
}

