// RUN: %check_safememory_instrument --fix-only %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers
// XFAIL: *

#include <utility>



struct TestObj {};

int function(int);
int function(int, TestObj&);
int function(int, TestObj&, TestObj&);
int function(int, TestObj&, TestObj&, TestObj&);

class Outer {
public:
    int arg0 = 5;
    template<class ... ARGS>
    void execute(ARGS&& ... args) {
        auto i = function(arg0, std::forward<ARGS>(args)...);
    // CHECK-FIXES: auto i = function(safememory::detail::dezombiefy( args )...);
    }
};

void dezombiefyParameterPack() {
    
    TestObj To;
    Outer o;

    o.execute();
    o.execute(To);
    o.execute(To, To);
    o.execute(To, To, To);
}


