// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>
#include "user_include.h"

template <class T>
T* templ_ptr_func(T* t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

template <class T>
T& templ_ref_func(T& t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

// this template is only instantiated with T as a value
template <class T>
auto templ_val_func(T t) {
    return t;
}

// this template is only instantiated with T as a ref
template <class T>
auto templ_val_func2(T t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

// this template is only instantiated with T as a ptr
template <class T>
auto templ_val_func3(T t) {
    return t;
// CHECK-FIXES: return nodecpp::safememory::dezombiefy( t );
}

void func(int* ip, int& ir) {

    templ_ptr_func(ip);
// CHECK-FIXES: templ_ptr_func(nodecpp::safememory::dezombiefy( ip ));

    templ_ref_func(ir);
// CHECK-FIXES: templ_ref_func(nodecpp::safememory::dezombiefy( ir ));

    templ_val_func(ir);
// CHECK-FIXES: templ_val_func(nodecpp::safememory::dezombiefy( ir ));

    templ_val_func2<const int&>(int(5));

    templ_val_func3(ip);
// CHECK-FIXES: templ_val_func3(nodecpp::safememory::dezombiefy( ip ));
}


