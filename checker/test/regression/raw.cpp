// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


void f() {
    int i = 0;
    int* ip = &i;

    int* ip2 = nullptr;
// CHECK: :[[@LINE-1]]:10: error: (RAW)

    int* ip3 = 0;
// CHECK: :[[@LINE-1]]:10: error: (RAW)

    *nullptr; // this is not valid C++
// CHECK: :[[@LINE-1]]:5: error: indirection requires pointer operand
}

void func(int* ip) {
    ip = nullptr;
// CHECK: :[[@LINE-1]]:8: error: (RAW)
}

int* badFunc() {
    return 0;
// CHECK: :[[@LINE-1]]:12: error: (RAW)
}


void f(int* p = nullptr);
// CHECK: :[[@LINE-1]]:13: error: (RAW)



int* badAgain() {
    int i = 0;
    int* ip = &i;
    if(i)
        return ip;
//CHECK: :[[@LINE-1]]:16: error: (S5.1)
    else
        return &i;
//CHECK: :[[@LINE-1]]:16: error: (S5.1)
}

void func51() {

	int i = 0;
	int* ptr1 = &i;
	{
    	int* ptr2 = &i;
		ptr2 = ptr1; // ok
		ptr1 = ptr2; // bad
//CHECK: :[[@LINE-1]]:8: error: (S5.1)
	}
}

int* change(int* ptr) {
    int i = 0;
    ptr = &i;
//CHECK: :[[@LINE-1]]:9: error: (S5.1)

    return ptr; //ok
}
