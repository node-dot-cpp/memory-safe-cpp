// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

using namespace nodecpp;

// user functions
nodecpp::awaitable<int> awaitInt();
void eatAwaitable(nodecpp::awaitable<void>);


nodecpp::awaitable<void> func() {

	co_await await_function(); // ok

	await_function();
// CHECK: :[[@LINE-1]]:2: error: (S9)


	nodecpp::awaitable<void> var1 = await_function();
	co_await var1; // ok

	auto avar = await_function();
	co_await avar; // ok


	nodecpp::awaitable<void> var2 = await_function();
// CHECK: :[[@LINE-1]]:27: error: (S9)

	eatAwaitable(await_function());
// CHECK: :[[@LINE-1]]:15: error: (S9)

	int i = co_await awaitInt(); //ok

	int j = (co_await awaitInt()) + 1;
// CHECK: :[[@LINE-1]]:11: error: (S9)


	nodecpp::awaitable<void> var3 = await_function();
	co_await nodecpp::wait_for_all(std::move(var3), await_function()); //all ok

	

	[[nodecpp::no_await]] no_await_function();

	no_await_function();
// CHECK: :[[@LINE-1]]:2: error: (S9)


	co_await no_await_function(); //this is ok

	[[nodecpp::no_await]] await_function();
// CHECK: :[[@LINE-1]]:24: error: (S9)

	co_return;
}

