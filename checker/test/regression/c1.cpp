// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

[[nodecpp::abcd]] void f();
// CHECK: :[[@LINE-1]]:3: error: (C1)

