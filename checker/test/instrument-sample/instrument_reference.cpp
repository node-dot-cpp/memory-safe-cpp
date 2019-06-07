// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc

#include <safe_ptr.h>

using namespace nodecpp::safememory;

struct TestOut;

struct TestIn {
    int value = 0;
    void getIn(TestOut& outter, TestIn& other);
};

struct TestOut {
    owning_ptr<TestIn> inner;
    owning_ptr<TestIn> inner2;

    void begin() {
        inner = make_owning<TestIn>();
        inner2 = make_owning<TestIn>();
        inner->getIn(*this, *inner2);
    }

    void getOut() {
        inner2.reset();
    }
};

void TestIn::getIn(TestOut& outter, TestIn& other) {
    outter.getOut();
 
    //at this point, 'other' is a zombie
    int v = other.value;
}

int main() {

    TestOut out;
    out.begin();

    return 0;
}

