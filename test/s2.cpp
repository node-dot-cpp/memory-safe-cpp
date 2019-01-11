// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"



class Bad {
	mutable int i;
// CHECK-MESSAGES: :[[@LINE-1]]:14: warning: (S2.2)
};

void bad() { 

	const int i = 0;

	int& j = const_cast<int&>(i);
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S2.1)

	auto l = []() mutable {};
// CHECK-MESSAGES: :[[@LINE-1]]:11: warning: (S2)

}
