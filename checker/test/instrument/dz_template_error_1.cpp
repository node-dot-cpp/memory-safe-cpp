// RUN: %check_nodecpp_instrument --no-silent-mode %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs

#include <dezombiefy.h>


struct TestObj {};
struct TestObj2 {};


template<class T>
class TemplateClass {
public:
    template<class U>
    class TemplateInner {
    public:
        
        auto aMethod(U u) {
//CHECK-MESSAGES: :[[@LINE-1]]:14: error: Template funcion 'aMethod' has inconsistent dezombiefy requirements
            return u;
        }

        template<class V>
        auto templMethod(V v) {
//CHECK-MESSAGES: :[[@LINE-1]]:14: error: Template funcion 'templMethod' has inconsistent dezombiefy requirements
            return v;
        }
    };
};


int main() {

    TestObj to;

    TemplateClass<TestObj2&>::TemplateInner<TestObj> tcVal;
    TemplateClass<TestObj&>::TemplateInner<TestObj&> tcRef;

     tcVal.aMethod(to);
     tcRef.aMethod(to);

    tcVal.templMethod<TestObj&>(to);
    tcRef.templMethod<TestObj>(to);
}