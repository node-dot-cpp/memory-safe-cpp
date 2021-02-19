// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>

class [[safememory::awaitable]] UserAwaitable {
// CHECK: :[[@LINE-1]]:33: error: (C2)
// CHECK: :[[@LINE-2]]:33: error: unsafe type

	bool await_ready() noexcept { return false;	}
	void await_suspend(std::experimental::coroutine_handle<> h_) noexcept {}
	void await_resume() { }

};

nodecpp::awaitable<void> func() {


	co_await nodecpp::hidden_await_function();
	
	nodecpp::hidden_await_function();
// CHECK: :[[@LINE-1]]:2: error: (S9)
	co_await nodecpp::bad_await_function();
	
	nodecpp::bad_await_function();
}


