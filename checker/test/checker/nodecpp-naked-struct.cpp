// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

using namespace safememory;

struct [[safememory::naked_struct]] NakedInner {
    nullable_ptr<long> l;
};


struct [[safememory::naked_struct]] Naked {
// CHECK: :[[@LINE-1]]:37: error: unsafe naked_struct declaration
    nullable_ptr<int> i; //ok
    int* bad1; //bad

    NakedInner inner;

    nullable_ptr<NakedInner> bad2; //TODO

    Naked& operator=(const Naked&) = default;
};

