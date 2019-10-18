// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>

struct TestObj {};

template <class T, int I>
T& templConstexprIf(T& t1, T& t2) {
    if constexpr (I == 0)
       return t1;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t1 );
    else
       return t2;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t2 );
}


void func() {

    TestObj val;

    templConstexprIf<TestObj, 0>(val, val);

    templConstexprIf<TestObj, 1>(val, val);
}


