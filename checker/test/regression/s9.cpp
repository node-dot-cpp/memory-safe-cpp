// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

nodecpp::awaitable<int> af();
nodecpp::awaitable<int> af2();

template <class T>
void another_f(T t) {}
// CHECK: :[[@LINE-1]]:18: error: (S9.1)


struct SomeClass {
// CHECK: :[[@LINE-1]]:8: error: unsafe type declaration

	nodecpp::awaitable<int> aw;

};

nodecpp::awaitable<void> func() {

	{ co_await af(); }
	{ int x = co_await af(); }
	
	{
	auto x = af();
// CHECK: :[[@LINE-1]]:7: error: (S9.1)
// CHECK: :[[@LINE-2]]:11: error: (S9.1)
	auto y = af2();
// CHECK: :[[@LINE-1]]:7: error: (S9.1)
// CHECK: :[[@LINE-2]]:11: error: (S9.1)
	co_await x;
	co_await y;
	}
	{
	nodecpp::awaitable<int> x = af();
// CHECK: :[[@LINE-1]]:26: error: (S9.1)
// CHECK: :[[@LINE-2]]:30: error: (S9.1)
	co_await x;
	}

	{ co_await nodecpp::wait_for_all(af(), af2()); }


	

	{ af(); }
// CHECK: :[[@LINE-1]]:4: error: (S9.1)
	{ auto x = af(); }
// CHECK: :[[@LINE-1]]:9: error: (S9.1)
// CHECK: :[[@LINE-2]]:13: error: (S9.1)
	
//	int x = af();
	{
		auto x = af();
// CHECK: :[[@LINE-1]]:8: error: (S9.1)
// CHECK: :[[@LINE-2]]:12: error: (S9.1)
		another_f(std::move(x)); /* where another_f() takes nodecpp::awaitable<> */
// CHECK: :[[@LINE-1]]:13: error: (S9.1)
// CHECK: :[[@LINE-2]]:23: error: (S9.1)
	}

}

