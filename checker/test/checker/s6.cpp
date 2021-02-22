// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


void func() {
	asm("mov al, 2");
// CHECK: :[[@LINE-1]]:2: error: (S6.1)
	__asm("mov al, 2");
// CHECK: :[[@LINE-1]]:2: error: (S6.1)
}

