// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"


void func() {
	asm("mov al, 2");
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S6.1)
	__asm("mov al, 2");
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S6.1)
}

