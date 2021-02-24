// RUN: safememory-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>

using namespace safememory;

struct Safe {
	int i = 0;
};

struct [[safememory::naked_struct]] NakedStr {
	nullable_ptr<Safe> s1;
	nullable_ptr<Safe> s2;

	NakedStr() = default;

	NakedStr& operator=(const NakedStr&) = default;
	NakedStr& operator=(NakedStr&&) = default;

	NakedStr(const NakedStr&) = default;
	NakedStr(NakedStr&&) = default;
};

void func99() {

	Safe s1;
	nullable_ptr<Safe> ptr1(&s1);
	{
		Safe s2;
		nullable_ptr<Safe> ptr2(&s2);
		ptr2 = ptr1; // ok
		ptr1 = ptr2; // bad
//CHECK: :[[@LINE-1]]:8: error: (S5.1)

	}

	NakedStr nak1;
	{
		NakedStr nak2;

		nak2 = nak1; // ok
		nak1 = nak2; // bad
//CHECK: :[[@LINE-1]]:8: error: (S5.1)

		nak1.s1 = ptr1; // ok

		nullable_ptr<Safe> ptr2;
		nak1.s2 = ptr2; // bad
//CHECK: :[[@LINE-1]]:11: error: (S5.1)

		nak2.s2 = ptr2; //ok
	}


}


struct Some {
	nullable_ptr<int> get();
	nullable_ptr<Some> join(nullable_ptr<Some> = nullable_ptr<Some>());
	nullable_ptr<long> operator>>(nullable_ptr<long> p) { return p; }
};

nullable_ptr<int> operator>>(Some& s, nullable_ptr<int> a) { return a; }

nullable_ptr<int> func(nullable_ptr<int> = nullable_ptr<int>());
nullable_ptr<int> func2(nullable_ptr<int> p1, nullable_ptr<int> p2);
nullable_ptr<int> func3(int, nullable_ptr<int>); // worry only about int*
nullable_ptr<int> func4(int&);

nullable_ptr<int> func5(Some&);

nullable_ptr<Some> func6(int&, nullable_ptr<Some>); //int& can't become Some*

nullable_ptr<int> func7(nullable_ptr<int>, nullable_ptr<char>); //don't worry about char*

nullable_ptr<char> func8(nullable_ptr<char>, nullable_ptr<const char>); //don't worry about const char*

void f1(nullable_ptr<int> arg) {
	nullable_ptr<int> p1;
	int i = 0;

	func(p1); //ok
	func(&i); //ok

	nullable_ptr<int> p2 = func(p1); //ok
	nullable_ptr<int> p3 = func(&i); //ok

	p1 = func(p1); //ok
	p1 = func(&i); //ok

	{	
		p1 = func(p2); //ok p1 and p2 have same life
	}

	p1 = func(nullable_ptr<int>()); //ok
	p1 = func(); //ok default arg used

	{
		p1 = func(arg); // argument are ok
	}

	nullable_ptr<int> goodp = func(func(p1)); 

	int good = *(func(p1));


	{
		int iBad = 0;
		p1 = func(&iBad); //bad
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	p1 = func2(p1, &i); // both args ok

	{	
		int iBad = 0;
		p1 = func2(p1, &iBad); // bad
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	{	
		int iBad = 0;
		p1 = func2(&iBad, p1); //bad 
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}
	{
		int i = 0; 
		p1 = func3(i, p1); //TODO, don't worry about value arg
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	{
		int i = 0;
		p1 = func4(i); //bad, worry about ref arg
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	{
		Some s;
		p1 = func5(s); //bad, assume Some can return int*
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	nullable_ptr<Some> sp;
	{
		int i = 0;
		sp = func6(i, sp); //TODO, don't worry about value arg
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	{
		nullable_ptr<char> cp;
		p1 = func7(p1, cp); //TODO, char* can't become int*
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	nullable_ptr<char> cp1;
	{
		nullable_ptr<const char> cp2;
		cp1 = func8(cp1, cp2); // TODO conts char can't become char*
// CHECK: :[[@LINE-1]]:7: error: (S5.1)
	}
}

void f2(nullable_ptr<Some> arg) {

	Some s;

	s.get(); //ok
	nullable_ptr<int> p2 = s.get(); //ok

	nullable_ptr<int> p1;
	p1 = s.get(); //ok

	nullable_ptr<Some> sp = &s;
	p1 = sp->get(); //ok

	{
		Some sInt;
		p1 = sInt.get(); //bad instance goes out of scope
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	sp = s.join(sp); //ok
	sp = s.join(arg); //ok
	sp = s.join();	//ok

	{
		Some sInt;
		sp = sInt.join(sp); //bad instance goes out of scope
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}

	{
		nullable_ptr<Some> ptrInt;
		sp = s.join(ptrInt); //bad argument goes out of scope
// CHECK: :[[@LINE-1]]:6: error: (S5.1)
	}
}

void f3() {


	nullable_ptr<int> p1;
	nullable_ptr<long> lp;
	Some s;
	{
		int i = 0;
		long l = 0;

		auto f = [](nullable_ptr<int> p, long) { return p; };

		p1 = f(p1, l); //TODO lambda goes out of scope, but captures are empty
// CHECK: :[[@LINE-1]]:6: error: (S5.1)

		p1 = f(&i, l); // bad i goes out of scope
// CHECK: :[[@LINE-1]]:6: error: (S5.1)




		p1 = (s >> p1); //ok function op

		p1 = (s >> &i); // bad function op
// CHECK: :[[@LINE-1]]:6: error: (S5.1)

		lp = s >> lp; // ok method op

		lp = s >> &l; // bad method op
// CHECK: :[[@LINE-1]]:6: error: (S5.1)

	}
}

nullable_ptr<int> f4() {
    int i = 0;
    nullable_ptr<int> np(&i);
    return np;
// CHECK: :[[@LINE-1]]:12: error: (S5.1)
}

NakedStr f5() {
	NakedStr ns;

	return ns;
// CHECK: :[[@LINE-1]]:9: error: (S5.1)
}

nullable_ptr<Safe> f6() {
	NakedStr ns;

	return ns.s1;
// CHECK: :[[@LINE-1]]:9: error: (S5.1)
}

int& f7() {
	int i = 0;
	return i;
// CHECK: :[[@LINE-1]]:9: error: (S5.1)
}
