// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"
// XFAIL: *

	template <class T>
	union MayBeBad {
		int i;
		T ptr;
	};

	template<class T>
	struct UsesUnion {
		MayBeBad<T> U;

		UsesUnion(int ii)  { U.i = ii; }
		T get() const { return U.ptr; }
	};

	void func(int) { 

		typedef UsesUnion<int*> BadThing;
		func(*BadThing(42).get());
	}
