// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;


owning_ptr<int> func() {
	owning_ptr<int> var = make_owning<int>();
	return var;
}

const int& func2(const int& i) {return i;}

void func3(const nullable_ptr<int>& i);
void func4(nullable_ptr<int>&);
// CHECK: :[[@LINE-1]]:30: error: (S5.3)


void func5(nullable_ptr<nullable_ptr<int>> i);
// CHECK: :[[@LINE-1]]:44: error: unsafe

int main() {
	const int& i2;
// CHECK: :[[@LINE-1]]:13: error: (S5.3)
// CHECK: :[[@LINE-2]]:13: error: declaration of reference variable

	const int& i = *func();
// CHECK: :[[@LINE-1]]:17: error: (S5.3)

	const int& j = func2(i + 1);
// CHECK: :[[@LINE-1]]:17: error: (S5.3)
}