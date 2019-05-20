// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -isystem %S/Inputs

#include <dezombiefy.h>

using namespace nodecpp;
using namespace nodecpp::safememory;

void func(int* ip, int& ir) {

    int* ip2 = ip;
// CHECK-FIXES: int* ip2 = nodecpp::safememory::dezombiefy( ip );

    int& ir2 = ir;
// CHECK-FIXES: int& ir2 = nodecpp::safememory::dezombiefy( ir );

    int i = *ip2;
// CHECK-FIXES: int i = *nodecpp::safememory::dezombiefy( ip2 );

    i = ir2;
// CHECK-FIXES: i = nodecpp::safememory::dezombiefy( ir2 );

//when dezombiefy is already there, don't instrument, 
    i = *nodecpp::safememory::dezombiefy(ip2); 
    i = *safememory::dezombiefy(ip2); 
    i = *dezombiefy(ip2); 

    i = i;
}

