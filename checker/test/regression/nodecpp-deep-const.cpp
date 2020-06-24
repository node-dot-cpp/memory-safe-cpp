// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>
#include <safe_memory/string.h>

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
struct [[nodecpp::deep_const]] DeepConstTemplate {
    T t;
};

void func() {

    DeepConstTemplate<soft_ptr<int>> b1;
// CHECK: :[[@LINE-1]]:38: error: unsafe deep_const attribute at variable declaration [deep-const]

    DeepConstTemplate<owning_ptr<DeepConst>> b2;
// CHECK: :[[@LINE-1]]:46: error: unsafe deep_const attribute at variable declaration [deep-const]


    DeepConstTemplate<owning_ptr<const DeepConst>> g1; //ok!

}


