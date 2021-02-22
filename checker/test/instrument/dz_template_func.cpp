// RUN: %check_safememory_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs



struct TestObj {};

template <class T>
T* templPtrAtDecl(T* t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

template <class T>
const T& templRefAtDecl(const T& t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
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
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

// this template is only instantiated with T as a ptr
template <class T>
auto templPtrOnly(T t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

// this template is instantiated with T as a ptr and ref
template <class T>
auto templRefAndPtr(T t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

//This template is has explicit specialization for ptr
template <class T>
auto templWithSpecialization(T t) {
    return t;
}

template <>
auto templWithSpecialization(TestObj* t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

//This template is never instantiated
template <class T>
T* templNoInstance(T* t) {
    return t;
}

void func() {

    TestObj val;

    templPtrAtDecl(&val);

    templRefAtDecl(val);

    templValueOnly(val);

    templRefOnly<const TestObj&>(val);

    templPtrOnly(&val);

    templRefAndPtr<const TestObj&>(val);
    templRefAndPtr(&val);

    templWithSpecialization(val);
    templWithSpecialization(&val);
}


