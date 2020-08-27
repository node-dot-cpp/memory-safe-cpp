// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>


// user functions
nodecpp::awaitable<int> awaitInt();
void eatAwaitable(nodecpp::awaitable<void>);


nodecpp::awaitable<void> func() {

	co_await nodecpp::await_function(); // ok

	nodecpp::await_function();
// CHECK: :[[@LINE-1]]:2: error: (S9)


	nodecpp::awaitable<void> var1 = nodecpp::await_function();
	co_await var1; // ok

	auto avar = nodecpp::await_function();
	co_await avar; // ok


	nodecpp::awaitable<void> var2 = nodecpp::await_function();
// CHECK: :[[@LINE-1]]:27: error: (S9)

	eatAwaitable(nodecpp::await_function());
// CHECK: :[[@LINE-1]]:15: error: (S9)

	int i = co_await awaitInt(); //ok

	int j = (co_await awaitInt()) + 1;
// CHECK: :[[@LINE-1]]:11: error: (S9)


	nodecpp::awaitable<void> var3 = nodecpp::await_function();
	co_await nodecpp::wait_for_all(std::move(var3), nodecpp::await_function()); //all ok

	
	nodecpp::no_await_function(); //this is ok

	co_await nodecpp::no_await_function(); //this is ok


	co_return;
}

