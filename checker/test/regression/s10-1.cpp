// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <string_literal.h>

using namespace nodecpp;

void f(string_literal sl);

void func() {

	const char* cp = "Hello world!";
// CHECK: :[[@LINE-1]]:14: error: (S1.3)
	string_literal a = "Hello world!";
	string_literal b = cp;
// CHECK: :[[@LINE-1]]:21: error: (S10.1)
	a = "Bye";
	a = cp;
// CHECK: :[[@LINE-1]]:4: error: (S10.1)

	f("Implicit");

	f(cp);
// CHECK: :[[@LINE-1]]:4: error: (S10.1)
}

