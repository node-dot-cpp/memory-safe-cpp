// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

// don't make much trouble around unnamed namespaces
namespace [[nodecpp::memory_unsafe]] {
	
//this is not checked by checker
static int* ptr = nullptr;

}

namespace [[nodecpp::memory_unsafe]] name1 {

}

namespace name1 {
// CHECK: :[[@LINE-1]]:11: error: (C3)
}

namespace name2 {

}

namespace  [[nodecpp::non_deterministic]] name2 {
// CHECK: :[[@LINE-1]]:43: error: (C3)
}

