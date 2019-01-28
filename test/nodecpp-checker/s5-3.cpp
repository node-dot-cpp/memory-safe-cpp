// RUN: nodecpp-checker %s -- -std=c++11 -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"


#include <safe_ptr.h>

using namespace nodecpp;

owning_ptr<int> func() {
	owning_ptr<int> var = make_owning<int>();
	return var;
}

int main() {
	const int& i = *func();
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: (S5.3)
}
