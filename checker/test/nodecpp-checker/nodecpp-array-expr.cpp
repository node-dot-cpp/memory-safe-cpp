// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs | FileCheck %s -implicit-check-not="{{warning|error}}:"

void f() { 
	int i;

	(&i)[2];
// CHECK: :[[@LINE-1]]:8: warning: (S1)
}
