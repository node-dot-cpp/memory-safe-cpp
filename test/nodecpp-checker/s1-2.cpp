// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;


struct Some {
    int i = 0;
};

void f() {
    int i = 0;
    *(&i);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.2)
    int* p = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: (S1.3)
    *p;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.2)

    Some s;
    (&s)->i;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S1.2)

    owning_ptr<Some> u;
    u->i; //ok
    u.get()->i; //ok
    *u; //ok
}

class Good {
	int i;
	int get() const {
		return this->i;//ok
	}
};
