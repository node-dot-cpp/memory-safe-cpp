// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>

using namespace nodecpp::safememory;

void normalFunc();

[[nodecpp::no_side_effect]]
int func(int i, int j) {
    return i + j;   
}

int func2(int, int);

[[nodecpp::no_side_effect]]
int func2(int i, int j) {
// CHECK: :[[@LINE-1]]:5: error: (C3)
    return i + j;   
}

[[nodecpp::no_side_effect]]
int func3();

int func3() {}
// CHECK: :[[@LINE-1]]:5: error: (C3)

struct Basic {};

struct Other {
    Other();
};


[[nodecpp::no_side_effect]]
void badFunc() {
    func(1, 2);//ok
    normalFunc();//bad
// CHECK: :[[@LINE-1]]:5: error: function with no_side_effect attribute can call only other no side effect functions [no-side-effect]

    Basic b;
    Other o;
// CHECK: :[[@LINE-1]]:11: error: function with no_side_effect attribute can call only other no side effect functions [no-side-effect]
}

