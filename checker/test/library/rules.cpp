// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_memory/safe_ptr.h>

using namespace safememory;


class X { public: virtual ~X() {} };

class Der :public X {};

void fp(X* p);

void fop(owning_ptr<X> op);

void func() {

    int i = 5;
    X* p = nullptr;
 // CHECK: :[[@LINE-1]]:12: error: (S1.2)
    X* p1 = nullptr;
// CHECK: :[[@LINE-1]]:13: error: (S1.2)
    X* p2 = nullptr;
// CHECK: :[[@LINE-1]]:13: error: (S1.2)
    nullable_ptr<X> np;
    soft_ptr<X> sp;
    owning_ptr<X> op;

    //[Rule S1]
    //PROHIBIT
//    (int*)p; on rule S1.1

    p^p2;
// CHECK: :[[@LINE-1]]:6: error: invalid operands to binary expression
    p+i;
// CHECK: :[[@LINE-1]]:6: error: (S1)
    p[i];
// CHECK: :[[@LINE-1]]:8: error: (S1)  
    p1=p2=p+i;
// CHECK: :[[@LINE-1]]:12: error: (S1)

//   *nullptr; on rule S1.2
//    *p; on rule S1.2
    
    //ALLOW
    dynamic_cast<Der*>(p);
    p=p2;
    // p=np.get_dereferencable();
    // p=sp.get().get_dereferencable();
    // p=op.get().get_dereferencable();
    fp(p);
    // fp(np.get_dereferencable());
    // fp(sp.get().get_dereferencable());
    // fp(op.get().get_dereferencable());
    &i;
    *np;
    *sp;
    *op;

    //[Rule S1.1]
    //PROHIBIT
    (int*)p;
// CHECK: :[[@LINE-1]]:5: error: (S1.1)
    static_cast<Der*>(p);
// CHECK: :[[@LINE-1]]:5: error: (S1.1)
    reinterpret_cast<int*>(p);
// CHECK: :[[@LINE-1]]:5: error: (S1.1)
//    soft_ptr_static_cast<X*>(p);


    //[Rule S1.2]
    //PROHIBIT
    *nullptr = 123;
// CHECK: :[[@LINE-1]]:5: error: indirection requires pointer operand
    *p;



    //[Rule S1.3]
    //PROHIBIT
    int* x = nullptr;
// CHECK: :[[@LINE-1]]:14: error: (S1.2)

    //[Rule S1.4]
    //PROHIBIT
    union Prohibit { nullable_ptr<X> x; int y; };
// CHECK: :[[@LINE-1]]:11: error: unsafe type declaration
// CHECK: :[[@LINE-2]]:38: note: (S1.4)

    //ALLOW
    union Allow { int x; long y; };
}

void rule_S2() {

    const X* cp = nullptr;
// CHECK: :[[@LINE-1]]:19: error: (S1.2)

    //[Rule S2.1]
    //PROHIBIT
    const_cast<X*>(cp);
// CHECK: :[[@LINE-1]]:5: error: (S2.1)

    //[Rule S2.2]
    //PROHIBIT
    class Prohibit2 { mutable int x = 0; };
// CHECK: :[[@LINE-1]]:35: error: (S2.2)
}

//rule S3

int x;
// CHECK: :[[@LINE-1]]:5: error: (S3)
thread_local int x2;
// CHECK: :[[@LINE-1]]:18: error: (S3)
static int x3;
// CHECK: :[[@LINE-1]]:12: error: (S3)


class ProhibitS3 {
    public:
    static int x;
// CHECK: :[[@LINE-1]]:16: error: (S3)    
};

void rule_S3() {

    static int x = 0;
// CHECK: :[[@LINE-1]]:16: error: (S3)
}


void rule_S4() {
    //PROHIBIT: 
    new X();
// CHECK: :[[@LINE-1]]:5: error: (S4)
    new int;
// CHECK: :[[@LINE-1]]:5: error: (S4)

    make_owning<X>();
// CHECK: :[[@LINE-1]]:5: error: (S4.1)
    soft_ptr<X> sp = make_owning<X>();
// CHECK: :[[@LINE-1]]:22: error: (S4.1)

    //ALLOW
    auto x1 = make_owning<X>();
    owning_ptr<X> x2 = make_owning<X>();
    fop(make_owning<X>());
}



void rule_S6() {
    //PROHIBIT: 
	asm("mov al, 2");
// CHECK: :[[@LINE-1]]:2: error: (S6.1)

	__asm("mov al, 2");
// CHECK: :[[@LINE-1]]:2: error: (S6.1)
}

void rule_S7() {
    //PROHIBIT: 

    auto fp = &rule_S7;
// CHECK: :[[@LINE-1]]:10: error: (S5.3)    
}


void rule_S8() {
    //PROHIBIT: 
	
	int i = 0;
	memset(&i,0,sizeof(i));
// CHECK: :[[@LINE-1]]:2: error: (S8)

    //ALLOW
    owning_ptr<X> x2 = make_owning<X>();// ok, safe library
	rule_S8(); // ok, defined in safe code
}
