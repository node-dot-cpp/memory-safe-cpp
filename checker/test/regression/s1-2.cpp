// RUN: nodecpp-checker -no-raw-ptr %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;


struct Some {
    int i = 0;
};

void f() {
    int i = 0;
    *(&i);
// CHECK: :[[@LINE-1]]:5: error: (S1.2)
    int* p = nullptr;
// CHECK: :[[@LINE-1]]:10: error: (S1.3)
    *p;
// CHECK: :[[@LINE-1]]:5: error: (S1.2)

    Some s;
    (&s)->i;
// CHECK: :[[@LINE-1]]:11: error: (S1.2)

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
