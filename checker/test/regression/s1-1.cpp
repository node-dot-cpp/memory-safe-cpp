// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

void bad1() { 
	unsigned i = 0;
	reinterpret_cast<void*>(i);
// CHECK: :[[@LINE-1]]:2: error: (S1.1)
	(void*)i;
// CHECK: :[[@LINE-1]]:2: error: (S1.1)
	void* p = nullptr;
// CHECK: :[[@LINE-1]]:12: error: (S1.2)

	static_cast<unsigned*>(p);
// CHECK: :[[@LINE-1]]:2: error: (S1.1)

}

void good1(int&& i) {
	((void) 0); //ok, definition of assert macro

	int&& j = static_cast<int&&>(i); //static cast to rvalue is ok

	short s1 = static_cast<long>(5); // cast of non pointer is ok
	short s2 = (long) 5; // cast of non pointer is ok
}
