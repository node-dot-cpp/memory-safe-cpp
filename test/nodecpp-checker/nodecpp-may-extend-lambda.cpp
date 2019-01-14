// RUN: nodecpp-checker %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <function_owned.h>

using namespace nodecpp;


class MyServer;

struct Socket {

	void onPlain(naked_ptr<Socket> sk [[nodecpp::may_extend_to_this]], naked_ptr<MyServer> my [[nodecpp::may_extend_to_this]] );
	void on(std::function<void()> cb [[nodecpp::may_extend_to_this]]);
};

struct SrvMember {
	void onPlain(naked_ptr<MyServer> my [[nodecpp::may_extend_to_this]]);
	void on(std::function<void(naked_ptr<Socket>)> cb [[nodecpp::may_extend_to_this]]);
};


struct MyServer {
	SrvMember srv;


	void main() {

		srv.onPlain(this);


		// srv.on([this](naked_ptr<Socket> sk [[nodecpp::owned_by_this]]){
		// 	sk->on([this, sk]() {
		// 		/* do something */
		// 	});
		// });
	}

	void onConnect(naked_ptr<Socket> sk [[nodecpp::owned_by_this]] ) {

		sk->onPlain(sk, this);


	}

	void onBad1(naked_ptr<Socket> ow1 [[nodecpp::owned_by_this]], naked_ptr<Socket> ow2 [[nodecpp::owned_by_this]] ) {

		ow1->onPlain(ow2, this); //bad
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: is not safe
		
	}

	void bad1(naked_ptr<Socket> dontExtend, naked_ptr<Socket> sock [[nodecpp::may_extend_to_this]]) {
		sock->on([this, sock]() { }); // ok, now 'sock is safe

		sock->on([this, dontExtend]() { }); //bad
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: unsafe capture to extend

		dontExtend->on([dontExtend]() { }); //good, as long as we don't have other captures

		dontExtend->on([this]() { }); //bad
// CHECK-MESSAGES: :[[@LINE-1]]:19: warning: capture of 'this' unsafe to extend scope
	}


	void bad2() {
		int i;
		auto l = [this, &i](naked_ptr<Socket> sock) {};
// CHECK-MESSAGES: :[[@LINE-1]]:20: warning: unsafe capture to extend scope
		srv.on(l);
// CHECK-MESSAGES: :[[@LINE-1]]:10: note: referenced from here
	} 

	void bad3(naked_ptr<Socket> sock) {
		sock->on([this]() { });// bad, sock is not a member
// CHECK-MESSAGES: :[[@LINE-1]]:13: warning: capture of 'this' unsafe to extend scope
	}

};






