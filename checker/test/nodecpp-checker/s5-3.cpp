// RUN: nodecpp-checker %s -- -std=c++14 -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;


owning_ptr<int> func() {
	owning_ptr<int> var = make_owning<int>();
	return var;
}

const int& func2(const int& i) {return i;}

int main() {
	const int& i = *func();
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: (S5.3)

	const int& j = func2(i + 1);
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: (S5.3)
}