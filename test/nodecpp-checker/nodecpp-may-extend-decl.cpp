// RUN: nodecpp-checker %s --checks=-*,nodecpp-may-extend-decl -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

class Sock {};

void f() {

    auto l = [](Sock* s [[nodecpp::may_extend_to_this]]) {}; //bad attribute not valid at lambda    
// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: attribute
}
