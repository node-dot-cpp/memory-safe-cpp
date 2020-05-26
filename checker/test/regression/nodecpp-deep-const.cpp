// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <safememory/string.h>

using namespace nodecpp::safememory;

struct [[nodecpp::deep_const]] DeepConst {
    int l;
    safememory::string str;
};

struct DeepConst;
// CHECK: :[[@LINE-1]]:8: error: (C3)


struct [[nodecpp::deep_const]] BadDeepConst {
// CHECK: :[[@LINE-1]]:32: error: unsafe deep_const attribute at declaration [deep-const]
    owning_ptr<long> ptr;
};


template <class T>
struct [[nodecpp::deep_const]] BadDeepConstTemplate {
    T t;
};

void func() {

    BadDeepConstTemplate<soft_ptr<int>> b;
// CHECK: :[[@LINE-1]]:41: error: unsafe deep_const attribute at variable declaration [deep-const]
}