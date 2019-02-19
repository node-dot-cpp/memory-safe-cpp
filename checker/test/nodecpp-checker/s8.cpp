// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

void func() {
	
	int i = 0;
	memset(&i,0,sizeof(i));
// CHECK: :[[@LINE-1]]:2: warning: (S8)


	func(); // ok, defined in safe code
}

