// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

[[nodecpp::memory_unsafe]] void f();
// CHECK: :[[@LINE-1]]:33: error: (C2)


[[nodecpp::non_deterministic]] constexpr int i = 0;
// CHECK: :[[@LINE-1]]:46: error: (C2)

union [[nodecpp::naked_struct]] Wrong {
// CHECK: :[[@LINE-1]]:33: error: (C2)
	int i;
	int j;
};


void func(int i [[nodecpp::may_extend_to_this]]);
// CHECK: :[[@LINE-1]]:15: error: (C2)

class Bad {
	void good(int i [[nodecpp::may_extend_to_this]]);//ok

	static void myStatic(int i [[nodecpp::may_extend_to_this]]);
// CHECK: :[[@LINE-1]]:27: error: (C2)
};

void g() {

    auto l = [](int i [[nodecpp::may_extend_to_this]]) {};
// CHECK: :[[@LINE-1]]:21: error: (C2)
}
