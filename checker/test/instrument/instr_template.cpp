// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -isystem %S/Inputs

#include <dezombify.h>

template <class T>
T&& some_func(T&& t) {
    return t;
// CHECK-FIXES: return nodecpp::dezombify( t );
}


void func(int* ip, int& ir) {

    int* ip2 = some_func(ip);
// CHECK-FIXES: int* ip2 = some_func(nodecpp::dezombify( ip ));

    int& ir2 = some_func(ir);
// CHECK-FIXES: int& ir2 = some_func(nodecpp::dezombify( ir ));

}

