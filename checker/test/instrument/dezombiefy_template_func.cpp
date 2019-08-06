// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>

struct TestObj {};

template <class T>
T* templPtrAtDecl(T* t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

template <class T>
const T& templRefAtDecl(const T& t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

// this template is only instantiated with T as a value
template <class T>
auto templValueOnly(T t) {
    return t;
}

// this template is only instantiated with T as a ref
template <class T>
auto templRefOnly(T t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

// this template is only instantiated with T as a ptr
template <class T>
auto templPtrOnly(T t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

// this template is instantiated with T as a ptr and ref
template <class T>
auto templRefAndPtr(T t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

//This template is specialized for reference
template <class T>
auto templWithSpecialization(T t) {
    return t;
}

template <>
auto templWithSpecialization(TestObj* t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

void func(TestObj* ptr, TestObj val) {

    templPtrAtDecl(ptr);
// CHECK-FIXES: templPtrAtDecl(nodecpp::safememory::dezombiefy( ptr ));

    templRefAtDecl(val);

    templValueOnly(val);

    templRefOnly<const TestObj&>(val);

    templPtrOnly(ptr);
// CHECK-FIXES: templPtrOnly(nodecpp::safememory::dezombiefy( ptr ));

    templRefAndPtr<const TestObj&>(val);
    templRefAndPtr(ptr);
// CHECK-FIXES: templRefAndPtr(nodecpp::safememory::dezombiefy( ptr ));

    templWithSpecialization(val);
    templWithSpecialization(ptr);
// CHECK-FIXES: templWithSpecialization(nodecpp::safememory::dezombiefy( ptr ));
}


