// RUN: safememory-checker %s | FileCheck %s -allow-empty -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

/* This file should not generate any warning */

using namespace safememory;

class X { };

void fop(owning_ptr<X> op) {

    if(op)
        return;

    auto x1 = make_owning<X>();
    fop(std::move(x1));
}
