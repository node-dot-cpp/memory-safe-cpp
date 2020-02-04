// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <nodecpp_error.h>



void bad() {
	try {

		throw 42;
// CHECK: :[[@LINE-1]]:3: error: (M1.1)
	}
	catch(int i) {
// CHECK: :[[@LINE-1]]:12: error: (M1.2)

	} 
	catch(nodecpp::error e) {
// CHECK: :[[@LINE-1]]:23: error: (M1.2)

	}
}

void good() {

	try {
		throw nodecpp::error(nodecpp::BAD_ERROR);
	}
	catch(nodecpp::error& byRef) {

	}
}
