// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <utility>
#include <awaitable.h>
#include <safe_ptr.h>

using namespace nodecpp::safememory;

nodecpp::awaitable<void> af();

nodecpp::awaitable<int> func() {

	int i = 0;
	{
		int& ir = i;
// CHECK: :[[@LINE-1]]:8: error: (S5.8)
		naked_ptr<int> np(i);
// CHECK: :[[@LINE-1]]:18: error: (S5.8)
		co_await af();
	}

	{
		//this is ok
		co_await af();
		int& ir = i;
		naked_ptr<int> np(i);
	}

	{
		//this is ok
		{
			int& ir = i;
			naked_ptr<int> np(i);
		}
		co_await af();
	}
}

