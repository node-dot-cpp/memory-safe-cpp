// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;

struct Safe1 {
	int i;
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


struct [[nodecpp::naked_struct]] NakedStr {
	naked_ptr<int> ptr;

	naked_ptr<int> get() const;
	NakedStr();
	NakedStr(const NakedStr&);
	NakedStr& operator=(const NakedStr&) = delete;
};

void nakedFunc() {
	
	int* i = nullptr; //bad
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: (S1.3)

	NakedStr naked; //ok
}

struct Bad1 {
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: unsafe type declaration
	int* ptr;
};

struct Bad2 : public NakedStr {
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: unsafe type declaration
};

struct Bad3 {
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: unsafe type declaration
	int* ptr;

	void set(int* ptr);
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: (S1.3)
};

void badFunc() {
	int** i = nullptr; //bad
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: (S1.3)
	NakedStr* nakedPtr = nullptr; // bad
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: (S1.3)
	Bad1 b1; //bad
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: unsafe type at variable declaration [nodecpp-var-decl]

	Bad2 b2; //bad
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: unsafe type at variable declaration [nodecpp-var-decl]

	Bad3 b3; //bad
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: unsafe type at variable declaration [nodecpp-var-decl]
}

class Sock {};

class Safe {

	void mayExtendCallback(Sock* dontExtend, Sock* sock [[nodecpp::may_extend_to_this]]) {
// CHECK-MESSAGES: :[[@LINE-1]]:31: warning: (S1.3)
// CHECK-MESSAGES: :[[@LINE-2]]:49: warning: (S1.3)
		Sock* other [[nodecpp::may_extend_to_this]] = sock;
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: (S1.3)
		Sock* other2 [[nodecpp::may_extend_to_this]] = dontExtend; //bad donExtend is not valid initializer
// CHECK-MESSAGES: :[[@LINE-1]]:9: warning: (S1.3)
	}

};
