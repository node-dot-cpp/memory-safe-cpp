// RUN: %check_safememory_instrument %s %t -- -- -std=c++17 -nostdinc -I%S -isystem %S/Inputs
// XFAIL: *




struct TestObj {};

int function();
int function(TestObj&);
int function(TestObj&, TestObj&);
int function(TestObj&, TestObj&, TestObj&);

template<class ... ARGS>
void execute(ARGS&& ... args) {
    auto i = function(args...);
// CHECK-FIXES: auto i = function(safememory::detail::dezombiefy( args )...);
}


void dezombiefyParameterPack() {
    
    TestObj To;

    execute();
    execute(To);
    execute(To, To);
    execute(To, To, To);
}


