// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

using namespace safememory;

class Aclass {
	void func() {

		(size_t)(void*)this;
// CHECK: :[[@LINE-1]]:3: error: (D1)
// CHECK: :[[@LINE-2]]:11: error: (S1.1)

		reinterpret_cast<size_t>(this);
// CHECK: :[[@LINE-1]]:3: error: (D1)

		bool good = static_cast<bool>(this);
	}
};