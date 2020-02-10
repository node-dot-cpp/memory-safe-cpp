// RUN: nodecpp-checker %s | FileCheck %s -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <server.h>

using namespace nodecpp::safememory;

void my_sort(std::function<void()>);

struct MyServer {
	nodecpp::SrvMember srv;

	void good() {
		srv.onEvent([this](naked_ptr<nodecpp::Socket> sock) {}); //is ok
	}

	void good2() {
		int i = 0;
		auto l = [this, &i](naked_ptr<nodecpp::Socket> sock) {};
		my_sort(l);
	} 

	void bad1() {
		int i = 0;
		auto l = [this, &i](naked_ptr<nodecpp::Socket> sock) {};
// CHECK: :[[@LINE-1]]:20: error: (S5.7)
		srv.onEvent(l);
	} 

	void bad2(naked_ptr<int> i) {

		auto l = [this, &i](naked_ptr<nodecpp::Socket> sock) {};
// CHECK: :[[@LINE-1]]:20: error: (S5.7)
		srv.onEvent(l);
	} 
};


class Bad {
	void bad(naked_ptr<nodecpp::SrvMember> srv) {
		auto l = [this](naked_ptr<nodecpp::Socket> sock) {};
// CHECK: :[[@LINE-1]]:13: error: (S5.7)
		srv->onEvent(l);
	}
};





