// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs




struct TestObj {};
struct TestObj2 {};


template<class T>
class TemplateClass {
public:

    class Inner {
    public:
        T attr;
        void set(T t) { attr = t; }

    };

    T method(T t);
};


template<class T>
T TemplateClass<T>::method(T t) {
    return t;
// CHECK-FIXES: return safememory::detail::dezombiefy( t );
}

int main() {

    TemplateClass<int*> ti;
    TemplateClass<double*> td;

    int* ip = nullptr;
    ti.method(ip);

    double* dp = nullptr;
    td.method(dp);
}