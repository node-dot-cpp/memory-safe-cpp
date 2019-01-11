// RUN: clang-tidy %s --checks=-*,nodecpp-raw-pointer-assignment -- -std=c++11 -nostdinc++ | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

void good1() { 
	int* p1 = nullptr; 
	int* p2 = nullptr; 
	p2 = p1; 
}

void good2() { 
	int* p1 = nullptr; 
	{ 
		int* p2 = nullptr; 
		p2 = p1;
	}
}

void bad1() {
	int* p1 = nullptr;
	{
		int* bad = nullptr;
		p1 = bad;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	} 
}

void good3() {
	int p1 = 0;
	{
		int* p2 = nullptr;
		p2 = &p1;
	}
}

void bad2() {
	int* p1 = nullptr;
	{
		int bad = 0;
		p1 = &bad;
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}
}

struct Some {
	int* get();
	Some* join(Some* other = nullptr);
	long* operator>>(long* p) { return p; }
};

int* operator>>(Some& s, int* a) { return a; }

int* func(int* p = nullptr);
int* func2(int* p1, int* p2);
int* func3(int, int*); // worry only about int*
int* func4(int&);

int* func5(Some&);

Some* func6(int&, Some*); //int& can't become Some*

int* func7(int*, char*); //don't worry about char*

char* func8(char*, const char*); //don't worry about const char*

void f1(int* arg) {
	int* p1 = nullptr;
	int i = 0;

	func(p1); //ok
	func(&i); //ok

	int* p2 = func(p1); //ok
	int* p3 = func(&i); //ok

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

	int* goodp = func(func(p1)); 

	int good = *(func(p1));


	{
		int iBad = 0;
		p1 = func(&iBad); //bad
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	p1 = func2(p1, &i); // both args ok

	{	
		int iBad = 0;
		p1 = func2(p1, &iBad); // bad
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	{	
		int iBad = 0;
		p1 = func2(&iBad, p1); //bad 
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}
	{
		int i = 0; 
		p1 = func3(i, p1); //TODO, don't worry about value arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	{
		int i = 0;
		p1 = func4(i); //bad, worry about ref arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	{
		Some s;
		p1 = func5(s); //bad, assume Some can return int*
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	Some* sp = nullptr;
	{
		int i = 0;
		sp = func6(i, sp); //TODO, don't worry about value arg
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	{
		char* cp = nullptr;
		p1 = func7(p1, cp); //TODO, char* can't become int*
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	char* cp1 = nullptr;
	{
		const char* cp2 = nullptr;
		cp1 = func8(cp1, cp2); // TODO conts char can't become char*
// CHECK-MESSAGES: :[[@LINE-1]]:7: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}
}

void f2(Some* arg) {
	Some s;

	s.get(); //ok
	int* p2 = s.get(); //ok

	int* p1 = nullptr;
	p1 = s.get(); //ok

	Some* sp = &s;
	p1 = sp->get(); //ok

	{
		Some sInt;
		p1 = sInt.get(); //bad instance goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	sp = s.join(sp); //ok
	sp = s.join(arg); //ok
	sp = s.join();	//ok

	{
		Some sInt;
		sp = sInt.join(sp); //bad instance goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}

	{
		Some* ptrInt = nullptr;
		sp = s.join(ptrInt); //bad argument goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]
	}
}

void f3() {


	int* p1 = nullptr;
	long* lp = nullptr;
	Some s;
	{
		int i = 0;
		long l = 0;

		auto f = [](int* p, long) { return p; };

		p1 = f(p1, l); //TODO lambda goes out of scope, but captures are empty
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]

		p1 = f(&i, l); // bad i goes out of scope
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]




		p1 = (s >> p1); //ok function op

		p1 = (s >> &i); // bad function op
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]

		lp = s >> lp; // ok method op

		lp = s >> &l; // bad method op
// CHECK-MESSAGES: :[[@LINE-1]]:6: warning: assignment of raw pointer may extend scope [nodecpp-raw-pointer-assignment]

	}
}