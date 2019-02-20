// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers | FileCheck %s -implicit-check-not="{{warning|error}}:"


void bad() {
	int* a = nullptr;
// CHECK: :[[@LINE-1]]:7: warning: (S1.3)

	a = a + 1;
// CHECK: :[[@LINE-1]]:8: warning: (S1)
	a = a - 1;
// CHECK: :[[@LINE-1]]:8: warning: (S1)
	a += 1;
// CHECK: :[[@LINE-1]]:4: warning: (S1)
	a -= 1;
// CHECK: :[[@LINE-1]]:4: warning: (S1)
	a++;
// CHECK: :[[@LINE-1]]:3: warning: (S1)
	++a;
// CHECK: :[[@LINE-1]]:2: warning: (S1)
	a--;
// CHECK: :[[@LINE-1]]:3: warning: (S1)
	--a;
// CHECK: :[[@LINE-1]]:2: warning: (S1)

	int b = 0;
	int c = (&b)[0];
// CHECK: :[[@LINE-1]]:16: warning: (S1)
}
