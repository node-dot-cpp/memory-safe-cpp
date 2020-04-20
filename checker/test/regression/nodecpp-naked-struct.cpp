// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;

struct [[nodecpp::naked_struct]] NakedInner {
    nullable_ptr<long> l;
};


struct [[nodecpp::naked_struct]] Naked {
// CHECK: :[[@LINE-1]]:34: error: unsafe naked_struct declaration
    nullable_ptr<int> i; //ok
    int* bad1; //bad

    NakedInner inner;

    nullable_ptr<NakedInner> bad2; //TODO

    Naked& operator=(const Naked&) = default;
};

