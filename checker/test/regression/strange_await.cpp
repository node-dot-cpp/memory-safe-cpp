// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"
// XFAIL: *

#include <utility>
#include <awaitable.h>

using namespace nodecpp;

nodecpp::awaitable<void> func() {


	co_await hidden_await_function();

}


