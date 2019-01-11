// RUN: clang-tidy %s -- -std=c++11 -nostdinc++ -isystem %S/Inputs | FileCheck %s -check-prefix=CHECK-MESSAGES -implicit-check-not="{{warning|error}}:"

#include <safe_ptr.h>
#include <function_owned.h>



using namespace nodecpp;



struct Socket {

	void on(std::function<void(bool)> cb);

};

struct Srv {

	void on(std::function<void(soft_ptr<Socket> socket)> cb);
	void on_owned(function_owned_arg0<void(soft_ptr<Socket> socket)> cb);

};

struct MyServer {
	Srv srv;

	void main() {
		auto l = [this](soft_ptr<Socket> socket) {/*   */};

		auto lo = [this](soft_ptr<Socket> socket [[nodecpp::owned_by_this]]) {/*   */};

		redirect_a(l);
		redirect_b(lo);

		redirect_owned_a(l);
		redirect_owned_b(lo);
	}

	template<class T> 
	void redirect_a(T t) {
		srv.on(t);

		std::function<void(soft_ptr<Socket>)> f;
		f = t;
	}

	template<class T> 
	void redirect_b(T t) {
		srv.on(t);
// CHECK-MESSAGES: :[[@LINE-1]]:10: warning: lambda with attribute
		std::function<void(soft_ptr<Socket>)> f;
		f = t;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: lambda with attribute
	}

	template<class T> 
	void redirect_owned_a(T t) {
		srv.on_owned(t);
// CHECK-MESSAGES: :[[@LINE-1]]:16: warning: lambda without attribute
		nodecpp::function_owned_arg0<void(soft_ptr<Socket>)> f;
		f = t;
// CHECK-MESSAGES: :[[@LINE-1]]:5: warning: lambda without attribute
	}

	template<class T> 
	void redirect_owned_b(T t) {
		srv.on_owned(t);

		nodecpp::function_owned_arg0<void(soft_ptr<Socket>)> f;
		f = t;
	}

	void full() {
		srv.on_owned([this](soft_ptr<Socket> socket [[nodecpp::owned_by_this]]) {
			socket->on([this, socket](bool hadError) {
				/*   */
			});
		});
	}
};
