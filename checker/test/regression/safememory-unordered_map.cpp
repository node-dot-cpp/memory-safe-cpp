// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <safememory/unordered_map.h>


using namespace nodecpp::safememory;

struct Safe1 {
	int i = 0;
};

struct Safe2 {
	owning_ptr<Safe1> s1Ptr;

	Safe1 s1;
};

class Safe3 :public Safe2 {};

void safeVector() {
	//all ok

	safememory::unordered_map<int, int> vi;

	safememory::unordered_map<int, Safe1> vS1;
	safememory::unordered_map<int, Safe2> vS2;
}


struct [[nodecpp::naked_struct]] NakedStr {
	nullable_ptr<int> ptr;

	nullable_ptr<int> get() const;
	NakedStr();
	NakedStr(const NakedStr&);
	NakedStr& operator=(const NakedStr&) = delete;
};

struct Bad1 {
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration
	nullable_ptr<int> ptr;
};



void badFunc() {
	safememory::unordered_map<int*, int*> vp; //bad
// CHECK: :[[@LINE-1]]:40: error: unsafe type at variable declaration

	safememory::unordered_map<int, NakedStr> vstr;
// CHECK: :[[@LINE-1]]:43: error: unsafe type at variable declaration

	safememory::unordered_map<int, Bad1> b1; //bad
// CHECK: :[[@LINE-1]]:39: error: unsafe type at variable declaration

}



template<class Key>
class [[nodecpp::deep_const]] SideEffectHash {
	size_t operator()(const Key&) {return 0;}
};

template<class Key>
class [[nodecpp::deep_const]] SideEffectEq {
	constexpr bool operator()(const Key&l, const Key&r) {return l == r;}
};

void badHashOrKeyEqual() {

	// hash is not safe
	safememory::unordered_map<int, int, NakedStr> bb0;
// CHECK: :[[@LINE-1]]:48: error: unsafe type at variable declaration

	// hash is safe but no deep_const
	safememory::unordered_map<int, int, Safe1> bb1;
// CHECK: :[[@LINE-1]]:45: error: unsafe type at variable declaration

	// deep_const but side effect
	safememory::unordered_map<int, int, SideEffectHash<int>> bb2;
// CHECK: :[[@LINE-1]]:59: error: unsafe type at variable declaration

	//key_equal es unsafe
	safememory::unordered_map<int, int, safememory::DefHash<int>, NakedStr> bb10;
// CHECK: :[[@LINE-1]]:74: error: unsafe type at variable declaration

	// key_equal is safe but no deep_const
	safememory::unordered_map<int, int, safememory::DefHash<int>, Safe1> bb11;
// CHECK: :[[@LINE-1]]:71: error: unsafe type at variable declaration

	safememory::unordered_map<int, int, safememory::DefHash<int>, SideEffectEq<int>> bb12;
// CHECK: :[[@LINE-1]]:83: error: unsafe type at variable declaration
}

