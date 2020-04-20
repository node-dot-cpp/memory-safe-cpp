// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <functional>

[[nodecpp::memory_unsafe]] void f();
// CHECK: :[[@LINE-1]]:33: error: (C2)


[[nodecpp::non_deterministic]] constexpr int i = 0;
// CHECK: :[[@LINE-1]]:46: error: (C2)

union [[nodecpp::naked_struct]] Wrong {
// CHECK: :[[@LINE-1]]:33: error: (C2)
	int i;
	int j;
};

class MayExtendBad {
    void onEvent(std::function<void()> cb [[nodecpp::may_extend_to_this]]);
// CHECK: :[[@LINE-1]]:40: error: (C2)
};

namespace [[nodecpp::memory_unsafe]] {
class MayExtendGood {
	// now this is ok on user side
    void onEvent(std::function<void()> cb [[nodecpp::may_extend_to_this]]);
};
}
