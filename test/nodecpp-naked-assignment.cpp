// RUN: nodecpp-checker %s --checks=-*,nodecpp-naked-assignment -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>

using namespace nodecpp;

struct Safe {
	int i = 0;
};

struct [[nodecpp::naked_struct]] NakedStr {
	naked_ptr<Safe> s1;
	naked_ptr<Safe> s2;

	NakedStr& operator=(const NakedStr&) = default;
	NakedStr& operator=(NakedStr&&) = default;
};

void func99() {

	Safe s1;
	naked_ptr<Safe> ptr1(&s1);
	{
		Safe s2;
		naked_ptr<Safe> ptr2(&s2);
		ptr2 = ptr1; // ok
		ptr1 = ptr2; // bad
//CHECK-MESSAGES: :[[@LINE-1]]:8: warning:

	}

	NakedStr nak1;
	{
		NakedStr nak2;

		nak2 = nak1; // ok
		nak1 = nak2; // bad
//CHECK-MESSAGES: :[[@LINE-1]]:8: warning:

		nak1.s1 = ptr1; // ok

		naked_ptr<Safe> ptr2;
		nak1.s2 = ptr2; // bad
//CHECK-MESSAGES: :[[@LINE-1]]:11: warning:

		nak2.s2 = ptr2; //ok
	}


}

using namespace nodecpp;

struct Some {
	naked_ptr<int> get();
	naked_ptr<Some> join(naked_ptr<Some> = nullptr);
	naked_ptr<long> operator>>(naked_ptr<long> p) { return p; }
};

naked_ptr<int> operator>>(Some& s, naked_ptr<int> a) { return a; }

naked_ptr<int> func(naked_ptr<int> = nullptr);
naked_ptr<int> func2(naked_ptr<int> p1, naked_ptr<int> p2);
naked_ptr<int> func3(int, naked_ptr<int>); // worry only about int*
naked_ptr<int> func4(int&);

naked_ptr<int> func5(Some&);

naked_ptr<Some> func6(int&, naked_ptr<Some>); //int& can't become Some*

naked_ptr<int> func7(naked_ptr<int>, naked_ptr<char>); //don't worry about char*

naked_ptr<char> func8(naked_ptr<char>, naked_ptr<const char>); //don't worry about const char*

void f1(naked_ptr<int> arg) {
	naked_ptr<int> p1;
	int i = 0;

	func(p1); //ok
	func(&i); //ok

	naked_ptr<int> p2 = func(p1); //ok
	naked_ptr<int> p3 = func(&i); //ok

	p1 = func(p1); //ok
	p1 = func(&i); //ok

	{	
		p1 = func(p2); //ok p1 and p2 have same life
	}

	p1 = func(nullptr); //ok
	p1 = func(); //ok default arg used

	{
		p1 = func(arg); // argument are ok
	}

	naked_ptr<int> goodp = func(func(p1)); 

	int good = *(func(p1));


	{
		int iBad = 0;
		p1 = func(&iBad); //bad
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	p1 = func2(p1, &i); // both args ok

	{	
		int iBad = 0;
		p1 = func2(p1, &iBad); // bad
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	{	
		int iBad = 0;
		p1 = func2(&iBad, p1); //bad 
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}
	{
		int i = 0; 
		p1 = func3(i, p1); //TODO, don't worry about value arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	{
		int i = 0;
		p1 = func4(i); //bad, worry about ref arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	{
		Some s;
		p1 = func5(s); //bad, assume Some can return int*
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	naked_ptr<Some> sp;
	{
		int i = 0;
		sp = func6(i, sp); //TODO, don't worry about value arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	{
		naked_ptr<char> cp;
		p1 = func7(p1, cp); //TODO, char* can't become int*
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	naked_ptr<char> cp1;
	{
		naked_ptr<const char> cp2;
		cp1 = func8(cp1, cp2); // TODO conts char can't become char*
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}
}

void f2(Some* arg) {
	Some s;

	s.get(); //ok
	naked_ptr<int> p2 = s.get(); //ok

	naked_ptr<int> p1;
	p1 = s.get(); //ok

	naked_ptr<Some> sp = &s;
	p1 = sp->get(); //ok

	{
		Some sInt;
		p1 = sInt.get(); //bad instance goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	sp = s.join(sp); //ok
	sp = s.join(arg); //ok
	sp = s.join();	//ok

	{
		Some sInt;
		sp = sInt.join(sp); //bad instance goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}

	{
		naked_ptr<Some> ptrInt;
		sp = s.join(ptrInt); //bad argument goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]
	}
}

void f3() {


	naked_ptr<int> p1;
	naked_ptr<long> lp;
	Some s;
	{
		int i = 0;
		long l = 0;

		auto f = [](naked_ptr<int> p, long) { return p; };

		p1 = f(p1, l); //TODO lambda goes out of scope, but captures are empty
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]

		p1 = f(&i, l); // bad i goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]




		p1 = (s >> p1); //ok function op

		p1 = (s >> &i); // bad function op
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]

		lp = s >> lp; // ok method op

		lp = s >> &l; // bad method op
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of naked_ptr may extend scope [nodecpp-naked-assignment]

	}
}
