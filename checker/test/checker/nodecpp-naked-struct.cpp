// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>

using namespace safe_memory;

struct [[safe_memory::naked_struct]] NakedInner {
    nullable_ptr<long> l;
};


struct [[safe_memory::naked_struct]] Naked {
// CHECK: :[[@LINE-1]]:38: error: unsafe naked_struct declaration
    nullable_ptr<int> i; //ok
    int* bad1; //bad

    NakedInner inner;

    nullable_ptr<NakedInner> bad2; //TODO

    Naked& operator=(const Naked&) = default;
};

