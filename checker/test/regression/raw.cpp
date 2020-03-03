// RUN: nodecpp-checker -raw-ptr %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


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
