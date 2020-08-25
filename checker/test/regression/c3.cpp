// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

// don't make much trouble around unnamed namespaces
namespace [[safe_memory::memory_unsafe]] {
	
//this is not checked by checker
static int* ptr = nullptr;

}

namespace [[safe_memory::memory_unsafe]] name1 {

}

namespace name1 {
// CHECK: :[[@LINE-1]]:11: error: (C3)
}

namespace name2 {

}

namespace  [[safe_memory::non_deterministic]] name2 {
// CHECK: :[[@LINE-1]]:47: error: (C3)
}

