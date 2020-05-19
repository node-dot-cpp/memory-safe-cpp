// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"
// XFAIL: *

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
    owning_ptr<long> ptr;
// CHECK: :[[@LINE-1]]:34: error: unsafe naked_struct declaration
};

void func() {

    BadDeepConst b;
}