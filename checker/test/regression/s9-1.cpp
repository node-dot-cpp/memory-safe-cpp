// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

nodecpp::awaitable<void> awaitVoid();

void eatAwaitable(nodecpp::awaitable<void>);

nodecpp::awaitable<int> awaitInt();

nodecpp::awaitable<void> func() {

	co_await awaitVoid(); // ok

	awaitVoid();
// CHECK: :[[@LINE-1]]:2: error: (S9.1)


	nodecpp::awaitable<void> var1 = awaitVoid();
	co_await var1; // ok

	nodecpp::awaitable<void> var2 = awaitVoid();
// CHECK: :[[@LINE-1]]:27: error: (S9.1)

	eatAwaitable(awaitVoid());
// CHECK: :[[@LINE-1]]:15: error: (S9.1)

	int i = co_await awaitInt(); //ok

	int j = (co_await awaitInt()) + 1;
// CHECK: :[[@LINE-1]]:27: error: (S9.1)


	nodecpp::awaitable<void> var3 = awaitVoid();
	co_await nodecpp::wait_for_all(std::move(var3), awaitVoid());

	co_return;
}

