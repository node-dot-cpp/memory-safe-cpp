// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"


class Good {
	int i = 0;
};
class Bad {
	long l;
};

class Bad2 {
	int x1 : 8;
};

template<class T>
struct MaybeBad {
	T t;
	T get() const { return t; }
};

void otherFunc(int);
void func() { 

	int good = 0;
	long bad;
// CHECK: :[[@LINE-1]]:7: error: (D2)

	Good g1;
	Bad b2;
// CHECK: :[[@LINE-1]]:6: error: (D2.1)
	Bad b3;
// CHECK: :[[@LINE-1]]:6: error: (D2.1)

	MaybeBad<int> b4;
// CHECK: :[[@LINE-1]]:16: error: (D2.1)

	otherFunc(MaybeBad<int>().get());
// CHECK: :[[@LINE-1]]:12: error: (D2.1)
}
