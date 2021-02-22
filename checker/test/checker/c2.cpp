// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <functional>

[[safememory::memory_unsafe]] void f();
// CHECK: :[[@LINE-1]]:36: error: (C2)


[[safememory::non_deterministic]] constexpr int i = 0;
// CHECK: :[[@LINE-1]]:49: error: (C2)

union [[safememory::naked_struct]] Wrong {
// CHECK: :[[@LINE-1]]:36: error: (C2)
	int i;
	int j;
};

class MayExtendBad {
    void onEvent(std::function<void()> cb [[safememory::may_extend_to_this]]);
// CHECK: :[[@LINE-1]]:40: error: (C2)
};

namespace [[safememory::memory_unsafe]] {
class MayExtendGood {
	// now this is ok on user side
    void onEvent(std::function<void()> cb [[safememory::may_extend_to_this]]);
};
}
