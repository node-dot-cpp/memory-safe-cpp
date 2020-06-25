// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>
#include <safe_memory/unordered_map.h>

using namespace safe_memory;

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

	unordered_map<int, int> vi;

	unordered_map<int, Safe1> vS1;
	unordered_map<int, Safe2> vS2;
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
	unordered_map<int*, int*> vp; //bad
// CHECK: :[[@LINE-1]]:28: error: unsafe type at variable declaration

	unordered_map<int, NakedStr> vstr;
// CHECK: :[[@LINE-1]]:31: error: unsafe type at variable declaration

	unordered_map<int, Bad1> b1; //bad
// CHECK: :[[@LINE-1]]:27: error: unsafe type at variable declaration

}


template<class Key>
struct BadHash {
	size_t operator()(const Key&) const {return 0;}
};

template<class Key>
struct [[nodecpp::deep_const]] SideEffectHash {
	size_t operator()(const Key&) const {return 0;}
};

template<class Key>
struct BadEq {
	constexpr bool operator()(const Key&l, const Key&r) const {return l == r;}
};

template<class Key>
struct [[nodecpp::deep_const]] SideEffectEq {
	constexpr bool operator()(const Key&l, const Key&r) const {return l == r;}
};


struct [[nodecpp::deep_const]] KeyWithSideEffectEqual {
	bool operator==(const KeyWithSideEffectEqual& o) const {return false;}
};

template<class Key>
struct [[nodecpp::deep_const]] GoodHash {
	[[nodecpp::no_side_effect]] std::size_t operator()(const Key&) const {return 0;}
};

void badHashOrKeyEqual() {

	// hash is no deep_const
	unordered_map<int, int, BadHash<int>> bb1;
// CHECK: :[[@LINE-1]]:33: error: unsafe type at variable declaration

	// deep_const but side effect
	unordered_map<int, int, SideEffectHash<int>> bb2;
// CHECK: :[[@LINE-1]]:47: error: unsafe type at variable declaration

	// key_equal is safe but no deep_const
	unordered_map<int, int, safe_memory::hash<int>, BadEq<int>> bb10;
// CHECK: :[[@LINE-1]]:52: error: unsafe type at variable declaration

	unordered_map<int, int, safe_memory::hash<int>, SideEffectEq<int>> bb12;
// CHECK: :[[@LINE-1]]:61: error: unsafe type at variable declaration

	//here the default safe_memory::equal_to<KeyWithSideEffectEqual> has side-effects
	unordered_map<KeyWithSideEffectEqual, int, GoodHash<KeyWithSideEffectEqual>> bb13;
// CHECK: :[[@LINE-1]]:79: error: unsafe type at variable declaration
}

