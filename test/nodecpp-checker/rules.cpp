// RUN: nodecpp-checker %s -- -std=c++11 -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;

class Base { public: virtual ~Base() {} };

class Der :public Base {};

void fp(Base* p);
// CHECK-MESSAGES: :[[@LINE-1]]:15: warning: (S1.3)
void func() {

    int i = 5;
    Base* p = nullptr;
 // CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S1.3)
    const Base* cp = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: (S1.3)
    Base* p1 = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S1.3)
    Base* p2 = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S1.3)
    naked_ptr<Base> np;
    soft_ptr<Base> sp;
    owning_ptr<Base> op;

    //[Rule S1]
    //PROHIBIT
//    (int*)p; on rule S1.1

    p^p2;
// CHECK-MESSAGES: :[[@LINE-1]]:6: error: invalid operands to binary expression
    p+i;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: (S1)
    p[i];
// CHECK-MESSAGES: :[[@LINE-1]]:8: warning: do not use index operator on unsafe types   
    p1=p2=p+i;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: (S1)

//   *nullptr; on rule S1.2
//    *p; on rule S1.2
    
    //ALLOW
    dynamic_cast<Der*>(p);
    p=p2;
    p=np.get();
    p=sp.get();
    p=op.get();
    fp(p);
    fp(np.get());
    fp(sp.get());
    fp(op.get());
    &i;
    *np;
    *sp;
    *op;

    //[Rule S1.1]
    //PROHIBIT
    (int*)p;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.1)
    static_cast<Der*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.1)
    reinterpret_cast<int*>(p);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.1)
//    soft_ptr_static_cast<X*>(p);


    //[Rule S1.2]
    //PROHIBIT
    *nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:5: error: indirection requires pointer operand
    *p;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1.2)


    //[Rule S1.3]
    //PROHIBIT
    int* x = nullptr;
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: (S1.3)

    //[Rule S1.4]
    //PROHIBIT
    union Prohibit { naked_ptr<Base> x; int y; };
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: unsafe type declaration
// CHECK-MESSAGES: :[[@LINE-2]]:38: note: (S1.4)

    //ALLOW
    union Allow { int x; long y; };



    //[Rule S2.1]
    //PROHIBIT
    const_cast<Base*>(cp);
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: (S1)
// CHECK-MESSAGES: :[[@LINE-2]]:5: warning: (S2.1)

    //[Rule S2.2]
    //PROHIBIT
    class Prohibit2 { mutable int x = 0; };
// CHECK-MESSAGES: :[[@LINE-1]]:35: warning: (S2.2)
}