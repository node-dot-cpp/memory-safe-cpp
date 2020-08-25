// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


void bad() {
	int i = 0;
	int* a = &i;

	a = a + 1;
// CHECK: :[[@LINE-1]]:8: error: (S1)
	a = a - 1;
// CHECK: :[[@LINE-1]]:8: error: (S1)
	a += 1;
// CHECK: :[[@LINE-1]]:4: error: (S1)
	a -= 1;
// CHECK: :[[@LINE-1]]:4: error: (S1)
	a++;
// CHECK: :[[@LINE-1]]:3: error: (S1)
	++a;
// CHECK: :[[@LINE-1]]:2: error: (S1)
	a--;
// CHECK: :[[@LINE-1]]:3: error: (S1)
	--a;
// CHECK: :[[@LINE-1]]:2: error: (S1)

	int b = 0;
	int c = (&b)[0];
// CHECK: :[[@LINE-1]]:16: error: (S1)
}
