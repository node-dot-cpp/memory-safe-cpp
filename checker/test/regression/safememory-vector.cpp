// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <safememory/vector.h>


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
	safememory::vector<int> vi;

	safememory::vector<Safe1> vS1;
	safememory::vector<Safe2> vS2;
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
	safememory::vector<int*> vp; //bad
// CHECK: :[[@LINE-1]]:27: error: unsafe type at variable declaration

	safememory::vector<NakedStr> vstr;
// CHECK: :[[@LINE-1]]:31: error: unsafe type at variable declaration

	safememory::vector<Bad1> b1; //bad
// CHECK: :[[@LINE-1]]:27: error: unsafe type at variable declaration
}

