// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


union Bad {
// CHECK: :[[@LINE-1]]:7: error: unsafe type declaration
	int i = 0;
	int* ptr;
};

void func1() { 
	Bad bad;
// CHECK: :[[@LINE-1]]:6: error: unsafe union at variable declaration
}


template <class T>
union MayBeBad {
	int i = 0;
	T ptr;
};

void func2() { 

	MayBeBad<long> good; //ok
	MayBeBad<long*> bad;
// CHECK: :[[@LINE-1]]:18: error: unsafe union at variable declaration
}

class UsesUnion {
// CHECK: :[[@LINE-1]]:7: error: unsafe type declaration
	MayBeBad<int*> pt;
};

void func3() {

	UsesUnion uu;
// CHECK: :[[@LINE-1]]:12: error: unsafe type at variable declaration

	UsesUnion();
// CHECK: :[[@LINE-1]]:2: error: unsafe type at temporary expression
}



