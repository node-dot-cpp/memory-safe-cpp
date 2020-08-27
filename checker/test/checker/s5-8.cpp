// RUN: nodecpp-checker --no-library-db %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>
#include <safe_memory/safe_ptr.h>

using namespace safe_memory;

nodecpp::awaitable<void> af();

nodecpp::awaitable<void> func() {

	int i = 0;
	{
		int& ir = i;
// CHECK: :[[@LINE-1]]:8: error: (S5.8)
		nullable_ptr<int> np(i);
// CHECK: :[[@LINE-1]]:21: error: (S5.8)
		co_await af();
	}

	{
		//this is ok
		co_await af();
		int& ir = i;
		nullable_ptr<int> np(i);
	}

	{
		//this is ok
		{
			int& ir = i;
			nullable_ptr<int> np(i);
		}
		co_await af();
	}
}

nodecpp::awaitable<int> func2() {

	int i = 0;
	{
		int& ir = i;
// CHECK: :[[@LINE-1]]:8: error: (S5.8)
		nullable_ptr<int> np(i);
// CHECK: :[[@LINE-1]]:21: error: (S5.8)
		co_yield i;
	}
	co_return 0;
}
