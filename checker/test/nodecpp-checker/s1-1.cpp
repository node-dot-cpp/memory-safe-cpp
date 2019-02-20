// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers | FileCheck %s -implicit-check-not="{{warning|error}}:"

void bad1() { 
	unsigned i = 0;
	reinterpret_cast<void*>(i);
// CHECK: :[[@LINE-1]]:2: warning: (S1.1)
	(void*)i;
// CHECK: :[[@LINE-1]]:2: warning: (S1.1)
	void* p = nullptr;
// CHECK: :[[@LINE-1]]:8: warning: (S1.3)
	static_cast<unsigned*>(p);
// CHECK: :[[@LINE-1]]:2: warning: (S1.1)

}

void good1(int&& i) {
	((void) 0); //ok, definition of assert macro

	int&& j = static_cast<int&&>(i); //static cast to rvalue is ok

	short s1 = static_cast<long>(5); // cast of non pointer is ok
	short s2 = (long) 5; // cast of non pointer is ok
}
