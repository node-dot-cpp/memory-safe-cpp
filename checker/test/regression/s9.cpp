// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

nodecpp::awaitable<int> af();
nodecpp::awaitable<int> af2();

template <class T>
void another_f(T t) {}



nodecpp::awaitable<void> func() {

	{ co_await af(); }
	{ int x = co_await af(); }
	{
	auto x = af();
	auto y = af2();
	co_await x;
	co_await y;
	}
	{
	nodecpp::awaitable<int> x = af();
	co_await x;
	}

//	{ co_await wait_for_all(af(), af2()); }


	

	{ af(); }
	{ auto x = af(); }
	
//	int x = af();
	{
		auto x = af();
		another_f(std::move(x)); /* where another_f() takes nodecpp::awaitable<> */
	}
	{
		auto x = af();
		auto y = std::move(x);
	}

// CHECK: :[[@LINE-1]]:2: warning: (S8)

}

