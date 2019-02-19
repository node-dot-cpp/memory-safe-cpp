// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs | FileCheck %s -implicit-check-not="{{warning|error}}:"

class Sock {};

void f() {

    auto l = [](Sock* s [[nodecpp::may_extend_to_this]]) {}; //bad attribute not valid at lambda    
// CHECK: :[[@LINE-1]]:23: warning: (S1.3)
// CHECK: :[[@LINE-2]]:23: warning: attribute
}
