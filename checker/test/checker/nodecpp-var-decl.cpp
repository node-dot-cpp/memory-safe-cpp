// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

using namespace safememory;


struct Safe1 {
	int i = 0;
};

struct Safe2 {
	owning_ptr<Safe1> s1Ptr;

	Safe1 s1;
};

class Safe3 :public Safe2 {};

void safeFun() {
	//all ok
	Safe1 s1;

	Safe2 s2;
	owning_ptr<Safe2> s2Ptr;

	Safe3 s3;
	owning_ptr<Safe3> s3Ptr;
}


struct [[safememory::naked_struct]] NakedStr {
	nullable_ptr<int> ptr;

	nullable_ptr<int> get() const;
	NakedStr();
	NakedStr(const NakedStr&);
	NakedStr& operator=(const NakedStr&) = delete;
};

void nakedFunc() {
	
	NakedStr naked; //ok
}

struct Bad1 {
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration
	int* ptr = nullptr;
// CHECK: :[[@LINE-1]]:13: error: (S1.2)
};

struct Bad2 : public NakedStr {
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration
};

struct Bad3 {
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration
	int* ptr = nullptr;
// CHECK: :[[@LINE-1]]:13: error: (S1.2)
	void set(int* ptr);

};

void badFunc() {
	int i0 = 0;
	int* i1 = &i0;
	int** i = &i1; //bad
// CHECK: :[[@LINE-1]]:8: error: (S5.3)

	NakedStr nstr;
	NakedStr* nakedPtr = &nstr; // bad
// CHECK: :[[@LINE-1]]:12: error: (S5.3)

	Bad1 b1; //bad
// CHECK: :[[@LINE-1]]:7: error: unsafe type at variable declaration
}

