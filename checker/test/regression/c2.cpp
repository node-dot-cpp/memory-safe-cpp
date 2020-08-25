// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <functional>

[[safe_memory::memory_unsafe]] void f();
// CHECK: :[[@LINE-1]]:37: error: (C2)


[[safe_memory::non_deterministic]] constexpr int i = 0;
// CHECK: :[[@LINE-1]]:50: error: (C2)

union [[safe_memory::naked_struct]] Wrong {
// CHECK: :[[@LINE-1]]:37: error: (C2)
	int i;
	int j;
};

class MayExtendBad {
    void onEvent(std::function<void()> cb [[safe_memory::may_extend_to_this]]);
// CHECK: :[[@LINE-1]]:40: error: (C2)
};

namespace [[safe_memory::memory_unsafe]] {
class MayExtendGood {
	// now this is ok on user side
    void onEvent(std::function<void()> cb [[safe_memory::may_extend_to_this]]);
};
}
