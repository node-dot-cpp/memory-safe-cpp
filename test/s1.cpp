// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"


void bad() {
	int* a = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: (S1.3)

	a = a + 1;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: (S1)
	a = a - 1;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: (S1)
	a += 1;
// CHECK-MESSAGES: :[[@LINE-1]]:4: warning: (S1)
	a -= 1;
// CHECK-MESSAGES: :[[@LINE-1]]:4: warning: (S1)
	a++;
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: (S1)
	++a;
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S1)
	a--;
// CHECK-MESSAGES: :[[@LINE-1]]:3: warning: (S1)
	--a;
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S1)

	int b = 0;
	int c = (&b)[0];
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: do not use index operator on unsafe types
}
