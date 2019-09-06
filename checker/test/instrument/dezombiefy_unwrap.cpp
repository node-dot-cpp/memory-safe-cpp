// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>

class SomeClass;

int getValue(SomeClass*);
SomeClass& getRef(SomeClass*);
int operator+(SomeClass& l, SomeClass& r);
int getNumber(int);

void func(SomeClass* l, SomeClass* r) {

    int j = getRef(l) + getRef(r);
// CHECK-FIXES: auto& nodecpp_0 = getRef(nodecpp::safememory::dezombiefy( l )); auto& nodecpp_1 = getRef(nodecpp::safememory::dezombiefy( r )); int j = nodecpp::safememory::dezombiefy( nodecpp_0 ) + nodecpp::safememory::dezombiefy( nodecpp_1 );
    int p = getValue(l) + getValue(r);
// CHECK-FIXES: auto&& nodecpp_2 = getValue(nodecpp::safememory::dezombiefy( l )); auto&& nodecpp_3 = getValue(nodecpp::safememory::dezombiefy( r )); int p = nodecpp_2 + nodecpp_3;
   
    getValue(l) + getValue(r);
// CHECK-FIXES: { auto&& nodecpp_0 = getValue(nodecpp::safememory::dezombiefy( l )); auto&& nodecpp_1 = getValue(nodecpp::safememory::dezombiefy( r )); nodecpp_0 + nodecpp_1; };

    if(getValue(l) + getValue(r) != 0)
// CHECK-FIXES: if(nodecpp::safememory::ne(nodecpp::safememory::add(getValue(nodecpp::safememory::dezombiefy( l )) , getValue(nodecpp::safememory::dezombiefy( r ))) , 0))
        int i = getValue(l) + getValue(r);
// CHECK-FIXES: { auto&& nodecpp_4 = getValue(nodecpp::safememory::dezombiefy( l )); auto&& nodecpp_5 = getValue(nodecpp::safememory::dezombiefy( r )); int i = nodecpp_4 + nodecpp_5; }

    int x = getNumber(5) + getNumber(6);//no pointer, no dezombiefy
}

