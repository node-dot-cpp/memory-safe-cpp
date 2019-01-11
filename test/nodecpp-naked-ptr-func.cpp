// RUN: clang-tidy %s --checks=-*,nodecpp-naked-ptr-func -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

void good1(int a);
void good2(int& a);
void good3(int* a);
int good4();
int& good5();
int* good6();

struct Good1 { 
	int good();
};

struct Good2 {
	int& good();
};

struct Good3 {
	int* good();
};

int*& bad1();
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: return type not allowed [nodecpp-naked-ptr-func]
int** bad2();
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: return type not allowed [nodecpp-naked-ptr-func]
void bad3(int** a);
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: parameter type not allowed [nodecpp-naked-ptr-func]
void bad4(int*& a);
// CHECK-MESSAGES: :[[@LINE-1]]:17: warning: parameter type not allowed [nodecpp-naked-ptr-func]
void bad5(int* a, int** b);
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: parameter type not allowed [nodecpp-naked-ptr-func]
void bad6(int* a, int*& b);
// CHECK-MESSAGES: :[[@LINE-1]]:25: warning: parameter type not allowed [nodecpp-naked-ptr-func]
