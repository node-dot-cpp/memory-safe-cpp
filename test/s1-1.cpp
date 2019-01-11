// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"


void bad1() { 
	size_t i = 0;
	reinterpret_cast<void*>(i);
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S1.1)
	(void*)i;
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S1.1)
	void* p = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: (S1.3)
	static_cast<size_t*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: (S1.1)

}

void good1(int&& i) {
	((void) 0); //ok, definition of assert macro

	int&& j = static_cast<int&&>(i); //static cast to rvalue is ok

	short s1 = static_cast<long>(5); // cast of non pointer is ok
	short s2 = (long) 5; // cast of non pointer is ok
}
