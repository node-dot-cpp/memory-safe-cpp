// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

[[safememory::abcd]] void f();
// CHECK: :[[@LINE-1]]:3: error: (C1)

