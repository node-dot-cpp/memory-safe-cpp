// RUN: %check_nodecpp_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>


struct TestObj {};

template<class T>
class TemplateClass {
public:
    auto& aMethod(T t) {
        return t;
    }

};


int main() {

    TestObj to;
    TemplateClass<TestObj&> tc;

    tc.aMethod(to);

}