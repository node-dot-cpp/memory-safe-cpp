// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"


union Bad {
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: unsafe type declaration
	int i;
	int* ptr;
};

void func1() { 
	Bad bad;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: unsafe union at variable declaration
}


template <class T>
union MayBeBad {
	int i;
	T ptr;
};

void func2() { 

	MayBeBad<long> good; //ok
	MayBeBad<long*> bad;
// CHECK-MESSAGES: :[[@LINE-1]]:18: warning: unsafe union at variable declaration
}

class UsesUnion {
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: unsafe type declaration
	MayBeBad<int*> pt;
};

void func3() { 

	UsesUnion uu;
// CHECK-MESSAGES: :[[@LINE-1]]:12: warning: unsafe type at variable declaration [nodecpp-var-decl]

	UsesUnion();
// CHECK-MESSAGES: :[[@LINE-1]]:2: warning: unsafe type at temporary expression
}



