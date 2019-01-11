// RUN: nodecpp-checker %s --checks=-*,nodecpp-naked-ptr-return -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"




int* bad1() {
    int i;
    return &i; //bad return address of local var
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: return of naked pointer may extend scope [nodecpp-naked-ptr-return]
}

int* good1(int* a) {
    return a;
}

int* good2(bool cond, int* a, int* b) {
    if(cond)
        return a;
    else
        return b;
}

int* good3(bool cond, int* a, int* b) {
    return cond ? a : b;
}

class Safe {
    int a;

    int* good() {
        return &a;
    }
};

