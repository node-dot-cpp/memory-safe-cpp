// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/string_literal.h>

using namespace safememory;

void f(string_literal sl);

void func() {

	// currently string_literal is not constructible from 'const char*'
	// and 'const char[]' is not a safe type.
	// So implicit convertion is not allowed by other rules.
	// Only explicit convertion matches rule 10.1

	const char* cp = "Hello world!";

	const char arr[] = "Hello world!";
// CHECK: :[[@LINE-1]]:13: error: unsafe type at variable declaration

	string_literal a = "Hello world!";

	string_literal b = cp;
// CHECK: :[[@LINE-1]]:17: error: no viable conversion

	string_literal c = string_literal(cp);
// CHECK: :[[@LINE-1]]:21: error: (S10.1)

	a = "Bye";

	a = cp;
// CHECK: :[[@LINE-1]]:4: error: no viable overloaded '='

	f("Implicit");

	f(cp);
// CHECK: :[[@LINE-1]]:2: error: no matching function for call
}

