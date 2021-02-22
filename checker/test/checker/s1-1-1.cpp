// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>

using namespace safememory;

void bad1() {

	soft_ptr<int> i;

	soft_ptr<int> i2 = soft_ptr_static_cast<int>( i );
// CHECK: :[[@LINE-1]]:21: error: (S1.1.1)

	soft_ptr<long> l = soft_ptr_reinterpret_cast<long>( i );
// CHECK: :[[@LINE-1]]:21: error: (S1.1.1)
}
