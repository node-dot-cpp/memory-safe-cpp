// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc -isystem %S/Inputs -isystem %S/../../3rdparty/clang/lib/Headers | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;


owning_ptr<int> func() {
	owning_ptr<int> var = make_owning<int>();
	return var;
}

const int& func2(const int& i) {return i;}

int main() {
	const int& i = *func();
// CHECK: :[[@LINE-1]]:17: warning: (S5.3)

	const int& j = func2(i + 1);
// CHECK: :[[@LINE-1]]:17: warning: (S5.3)
}