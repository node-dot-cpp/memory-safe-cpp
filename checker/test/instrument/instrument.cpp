// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -isystem %S/Inputs

#include <dezombify.h>

void func(int* ip, int& ir) {

    int* ip2 = ip;
// CHECK-FIXES: int* ip2 = nodecpp::dezombify( ip );

    int& ir2 = ir;
// CHECK-FIXES: int& ir2 = nodecpp::dezombify( ir );

    int i = *ip2;
// CHECK-FIXES: int i = *nodecpp::dezombify( ip2 );

    i = ir2;
// CHECK-FIXES: i = nodecpp::dezombify( ir2 );

//when dezombify is already there, don't instrument, 
    int i = *nodecpp::dezombify(ip2); 

    i = i;
}

