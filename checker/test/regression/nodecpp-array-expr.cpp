// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

void f() { 
	int i;

	(&i)[2];
// CHECK: :[[@LINE-1]]:8: error: (S1)
}
