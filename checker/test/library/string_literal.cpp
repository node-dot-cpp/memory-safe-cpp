// RUN: safememory-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/string_literal.h>
#include <safememory/string.h>

using namespace safememory;

void f(string_literal sl);

void func() {

	const char* cp = "Hello world!";

	const char arr[] = "Hello world!";
// CHECK: :[[@LINE-1]]:13: error: unsafe type at variable declaration

	string_literal a = "Hello world!";

	string_literal b = cp;
// CHECK: :[[@LINE-1]]:17: error: no viable conversion

	string_literal c = string_literal(cp);
// CHECK: :[[@LINE-1]]:21: error: no matching conversion

	string_literal d = string_literal(arr);
// CHECK: :[[@LINE-1]]:21: error: (S10.1)

	f("Implicit");

	f(cp);
// CHECK: :[[@LINE-1]]:2: error: no matching function for call


	string str0 = string("real literal");

	string str1 = string(cp);
// CHECK: :[[@LINE-1]]:16: error: (S10.1)

	string str2 = string(arr);
// CHECK: :[[@LINE-1]]:16: error: (S10.1)
}

