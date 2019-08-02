// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc

void func(int* ip, int& ir) {

    int* ip2 = ip;
// CHECK-FIXES: int* ip2 = nodecpp::safememory::dezombiefy( ip );

    int& ir2 = ir;
// CHECK-FIXES: int& ir2 = nodecpp::safememory::dezombiefy( ir );

    int i = *ip2;
// CHECK-FIXES: int i = *nodecpp::safememory::dezombiefy( ip2 );

    i = ir2;
// CHECK-FIXES: i = nodecpp::safememory::dezombiefy( ir2 );

}

class Class {

    int attribute = 0;

    void method() {

        int i = 0;
        i = attribute;
// CHECK-FIXES: i = nodecpp::safememory::dezombiefy( this )->attribute;
        i = this->attribute;
// CHECK-FIXES: i = nodecpp::safememory::dezombiefy( this )->attribute;

        method();
// CHECK-FIXES: nodecpp::safememory::dezombiefy( this )->method();
        this->method();
// CHECK-FIXES: nodecpp::safememory::dezombiefy( this )->method();

    }

};