// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/string_literal.h>

using namespace safememory;

void f(string_literal sl);

void func() {

	const char* cp = "Hello world!";

	string_literal a = "Hello world!";
	string_literal b = cp;
// CHECK: :[[@LINE-1]]:21: error: (S10.1)
	a = "Bye";
	a = cp;
// CHECK: :[[@LINE-1]]:6: error: (S10.1)

	f("Implicit");

	f(cp);
// CHECK: :[[@LINE-1]]:4: error: (S10.1)
}

