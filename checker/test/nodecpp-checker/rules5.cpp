// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp::safememory;

class X { public: virtual ~X() {} };

class Der :public X {};


naked_ptr<int> rule_S51() {
    int i = 0;
    naked_ptr<int> np(i);
    return np;
// CHECK: :[[@LINE-1]]:12: warning: (S5.1)
}

int* ruleX() {
    int i = 0;
    return &i;
// CHECK: :[[@LINE-1]]:12: warning: (S5.1)
}

naked_ptr<int> ff(naked_ptr<int> left, naked_ptr<int> right);


void rule_S52() {

    //PROHIBIT
    {
        naked_ptr<int> i1;
        {
            naked_ptr<int> i2;
            i1 = ff(i1, i2);
// CHECK: :[[@LINE-1]]:16: warning: (S5.2)
        }
    }

    //ALLOW
    {
        naked_ptr<int> i1;
        {
            naked_ptr<int> i2;
            i2 = ff(i1, i2);
        }
    }
}


//PROHIBIT
void ff(naked_ptr<int>& x);
// CHECK: :[[@LINE-1]]:25: warning: (S5.3)


//ALLOW

void ff(naked_ptr<int> np);
//void ff(const_naked_ptr<int>& np);


void rule_S53() {

    //PROHIBIT
    int* p = nullptr;
// CHECK: :[[@LINE-1]]:10: warning: (S1.3)
    int** ipp = nullptr;
// CHECK: :[[@LINE-1]]:11: warning: (S1.3)
    int*& x = p;
// CHECK: :[[@LINE-1]]:11: warning: (S5.3)

    naked_ptr<int*> npp;
// CHECK: :[[@LINE-1]]:21: warning: unsafe naked_ptr at variable declaration


    //ALLOW
//    const int *& x = p;
}

//rule S5.4



//PROHIBIT

struct X2 { naked_ptr<int> y; };
// CHECK: :[[@LINE-1]]:8: warning: unsafe type declaration

struct [[nodecpp::naked_struct]] NSTR { naked_ptr<int> y; };
struct [[nodecpp::naked_struct]] NSTR2 { soft_ptr<NSTR> y; };
// CHECK: :[[@LINE-1]]:34: warning: unsafe naked_struct declaration

void rule_S54() {

    owning_ptr<NSTR> nstr = make_owning<NSTR>();
// CHECK: :[[@LINE-1]]:22: warning: unsafe type at variable declaration
}

//ALLOW

struct X1 { soft_ptr<int> y; };

struct [[nodecpp::naked_struct]] NSTR1 { naked_ptr<int> y; };


//rule S5.5
//PROHIBIT:
void ff55(NSTR&);
// CHECK: :[[@LINE-1]]:16: warning: (S5.3)

void func_S55() {
    naked_ptr<NSTR> nstr;
// CHECK: :[[@LINE-1]]:21: warning: unsafe naked_ptr at variable declaration
}

//ALLOW
void ff(const NSTR&);
// CHECK: :[[@LINE-1]]:20: warning: (S5.3)

void func_S55A() {
//    const_naked_ptr<NSTR> cnstr;
}

