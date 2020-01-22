// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;


owning_ptr<int> func() {
	owning_ptr<int> var = make_owning<int>();
	return var;
}

const int& func2(const int& i) {return i;}

void func3(const naked_ptr<int>& i);
void func4(naked_ptr<int>&);
// CHECK: :[[@LINE-1]]:27: error: (S5.3)


void func5(naked_ptr<naked_ptr<int>> i);
// CHECK: :[[@LINE-1]]:38: error: unsafe naked_ptr at variable

int main() {
	const int& i2;
// CHECK: :[[@LINE-1]]:13: error: (S5.3)
// CHECK: :[[@LINE-2]]:13: error: declaration of reference variable

	const int& i = *func();
// CHECK: :[[@LINE-1]]:17: error: (S5.3)

	const int& j = func2(i + 1);
// CHECK: :[[@LINE-1]]:17: error: (S5.3)
}