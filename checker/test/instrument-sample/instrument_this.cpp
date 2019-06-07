// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc

#include <safe_ptr.h>
#include <safe_memory_error.h>
#include <stdio.h>

using namespace nodecpp::safememory;

struct TestOut;

struct TestIn {
    int value = 0;
    void getIn(TestOut& outter);
};

struct TestOut {
    owning_ptr<TestIn> inner;

    void begin() {
        inner = make_owning<TestIn>();
        inner->getIn(*this);
    }

    void getOut() {
        inner.reset();
    }
};

void TestIn::getIn(TestOut& outter) {
    outter.getOut();
 
    //at this point, 'this' is a zombie
    int v = value;
}

int main() {

    try {
        TestOut out;
        out.begin();
    }
    catch(nodecpp::error::memory_error&) {
        printf("\nZombie catched!\n");
        return 0;
    }

    printf("\nZombie bit you.\n");
    return 0;
}

