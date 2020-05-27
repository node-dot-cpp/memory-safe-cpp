// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

using namespace nodecpp;

class [[nodecpp::awaitable]] UserAwaitable {
// CHECK: :[[@LINE-1]]:30: error: (C2)
// CHECK: :[[@LINE-2]]:30: error: unsafe type

	bool await_ready() noexcept { return false;	}
	void await_suspend(std::experimental::coroutine_handle<> h_) noexcept {}
	void await_resume() { }

};

nodecpp::awaitable<void> func() {


	co_await hidden_await_function();
	
	hidden_await_function();
// CHECK: :[[@LINE-1]]:2: error: (S9)
	co_await bad_await_function();
	
	bad_await_function();
}


