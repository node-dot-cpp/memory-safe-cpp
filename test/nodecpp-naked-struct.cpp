// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;

struct [[nodecpp::naked_struct]] NakedInner {
    naked_ptr<long> l;
};


struct [[nodecpp::naked_struct]] Naked {
// CHECK-MESSAGES: :[[@LINE-1]]:34: warning: unsafe naked_struct declaration
    naked_ptr<int> i; //ok
    int* bad1; //bad

    NakedInner inner;

    naked_ptr<NakedInner> bad2; //TODO

    Naked& operator=(const Naked&) = default;
};

