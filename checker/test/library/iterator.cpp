// RUN: safememory-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safememory/safe_ptr.h>
#include <safememory/vector.h>
#include <safememory/unordered_map.h>


using namespace safememory;

vector<int>::iterator itFunc(vector<int>::iterator in) {
	return in;
}

vector<int>::iterator itFunc2() {
	vector<int> vi;
	return vi.end();
// CHECK: :[[@LINE-1]]:9: error: (S5.1) return value may extend scope
}

void vectorIterator() {
	//all ok
	vector<int>::iterator it;
	vector<int>::iterator_safe sit;
	{
		vector<int> vi;
		it = vi.end();
// CHECK: :[[@LINE-1]]:6: error: (S5.1) assignment may extend scope

		sit = vi.end_safe();//ok

		it = itFunc(vi.end());
// CHECK: :[[@LINE-1]]:6: error: (S5.1) assignment may extend scope

		sit = vi.make_safe(vi.end());
	}	 
}


unordered_map<int,int>::iterator mapFunc(unordered_map<int,int>::iterator in) {
	return in;
}

unordered_map<int,int>::iterator mapFunc2() {
	unordered_map<int,int> vi;
	return vi.end();
// CHECK: :[[@LINE-1]]:9: error: (S5.1) return value may extend scope
}

void mapIterator() {
	//all ok
	unordered_map<int,int>::iterator it;
	unordered_map<int,int>::iterator_safe sit;
	{
		unordered_map<int,int> vi;
		it = vi.end();
// CHECK: :[[@LINE-1]]:6: error: (S5.1) assignment may extend scope

		sit = vi.end_safe();//ok

		it = mapFunc(vi.end());
// CHECK: :[[@LINE-1]]:6: error: (S5.1) assignment may extend scope

		sit = vi.make_safe(vi.end());
	}	 
}


