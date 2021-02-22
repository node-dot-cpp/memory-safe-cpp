// RUN: safememory-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>
#include <safememory/vector.h>


using namespace safememory;

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
	vector<int> vi;

	vector<Safe1> vS1;
	vector<Safe2> vS2;
}


struct [[safememory::naked_struct]] NakedStr {
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
	vector<int*> vp; //bad
// CHECK: :[[@LINE-1]]:15: error: unsafe type at variable declaration

	vector<NakedStr> vstr;
// CHECK: :[[@LINE-1]]:19: error: unsafe type at variable declaration

	vector<Bad1> b1; //bad
// CHECK: :[[@LINE-1]]:15: error: unsafe type at variable declaration
}

