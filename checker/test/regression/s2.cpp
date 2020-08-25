// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"



class Bad {
	mutable int i;
// CHECK: :[[@LINE-1]]:14: error: (S2.2)
};

void bad() { 

	const int i = 0;

	int& j = const_cast<int&>(i);
// CHECK: :[[@LINE-1]]:11: error: (S2.1)

}
