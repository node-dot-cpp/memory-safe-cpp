// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>
#include <deep_const_type.h>

using namespace safe_memory;

struct [[safe_memory::deep_const]] DeepConst {
    int l;
    nodecpp::deep_const_type str;
};

struct DeepConst;
// CHECK: :[[@LINE-1]]:8: error: (C3)


struct [[safe_memory::deep_const]] BadDeepConst {
// CHECK: :[[@LINE-1]]:36: error: unsafe deep_const attribute at declaration [deep-const]
    owning_ptr<long> ptr;
};


template <class T>
struct [[safe_memory::deep_const]] DeepConstTemplate {
    T t;
};

void func() {

    DeepConstTemplate<soft_ptr<int>> b1;
// CHECK: :[[@LINE-1]]:38: error: unsafe deep_const attribute at variable declaration [deep-const]

    DeepConstTemplate<owning_ptr<DeepConst>> b2;
// CHECK: :[[@LINE-1]]:46: error: unsafe deep_const attribute at variable declaration [deep-const]


    DeepConstTemplate<owning_ptr<const DeepConst>> g1; //ok!

}

