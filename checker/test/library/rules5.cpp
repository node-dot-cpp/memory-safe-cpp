// RUN: safememory-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

using namespace safememory;

class X { public: virtual ~X() {} };

class Der :public X {};


nullable_ptr<int> rule_S51() {
    int i = 0;
    nullable_ptr<int> np(&i);
    return np;
// CHECK: :[[@LINE-1]]:12: error: (S5.1)
}

int* ruleX() {
    int i = 0;
    return &i;
// CHECK: :[[@LINE-1]]:12: error: (S5.1)
}

nullable_ptr<int> ff(nullable_ptr<int> left, nullable_ptr<int> right);


void rule_S52() {

    //PROHIBIT
    {
        nullable_ptr<int> i1;
        {
            nullable_ptr<int> i2;
            i1 = ff(i1, i2);
// CHECK: :[[@LINE-1]]:16: error: (S5.1)
        }
    }

    //ALLOW
    {
        nullable_ptr<int> i1;
        {
            nullable_ptr<int> i2;
            i2 = ff(i1, i2);
        }
    }
}


//PROHIBIT
void ff(nullable_ptr<int>& x);
// CHECK: :[[@LINE-1]]:28: error: (S5.3)


//ALLOW

void ff(nullable_ptr<int> np);
//void ff(const_nullable_ptr<int>& np);


void rule_S53() {

    int i = 0;
    int* p = &i;
    //PROHIBIT
    int** ipp = &p;
// CHECK: :[[@LINE-1]]:11: error: (S5.3)
    int*& x = p;
// CHECK: :[[@LINE-1]]:11: error: (S5.3)

    nullable_ptr<int*> npp;
// CHECK: :[[@LINE-1]]:24: error: unsafe nullable_ptr at variable declaration


    //ALLOW
//    const int *& x = p;
}

//rule S5.4



//PROHIBIT

struct X2 { nullable_ptr<int> y; };
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration

struct [[safememory::naked_struct]] NSTR { nullable_ptr<int> y; };
struct [[safememory::naked_struct]] NSTR2 { soft_ptr<NSTR> y; };
// CHECK: :[[@LINE-1]]:37: error: unsafe naked_struct declaration

void rule_S54() {

    owning_ptr<NSTR> nstr = make_owning<NSTR>();
// CHECK: :[[@LINE-1]]:22: error: unsafe type at variable declaration
}

//ALLOW

struct X1 { soft_ptr<int> y; };

struct [[safememory::naked_struct]] NSTR1 { nullable_ptr<int> y; };


//rule S5.5
//PROHIBIT:
void ff55(NSTR&);
// CHECK: :[[@LINE-1]]:16: error: (S5.3)

void func_S55() {
    nullable_ptr<NSTR> nstr;
// CHECK: :[[@LINE-1]]:24: error: unsafe nullable_ptr at variable declaration
}

//ALLOW
void ff(const NSTR&);


void func_S55A() {
//    const_nullable_ptr<NSTR> cnstr;
}

