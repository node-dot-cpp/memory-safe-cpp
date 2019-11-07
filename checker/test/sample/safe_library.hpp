
namespace fmt {
namespace v5 {
	void print();
	void format();
}	
}

namespace std {
	void move();
	void forward();
	void make_pair();//CHECK

//	class pair;
	class exception;

	class vector; //TODO
	class map;
	class basic_string;

	class _Tree_iterator;
	class _Vector_iterator;

	void operator==(void*, vector);

	// apple gcc std lib uses namespace __1
	namespace __1 {
		void move();
		void forward();
	}

	namespace experimental {
		namespace coroutines_v1 {
			class coroutine_handle;
			void operator!=(void*, coroutine_handle);
		}
	}

}

namespace nodecpp {
	void isException();
	void getException();
	void setException();

	void setNoException();
	void setTimeoutForAction();
	void clearTimeout();

namespace safememory {
	void make_owning();
	void soft_ptr_static_cast();
	// osn ptrs are hardcoded with special safety rules
	// class owning_ptr;
	// class soft_ptr;
	// class naked_ptr;
}
namespace log {
	void log();
}
namespace assert {
	void nodecpp_assert();
}

class Buffer;

namespace net {
	class Socket;
	class Server;
	class Address;
	class SocketTBase;


	class UserDefHandlersWithOptimizedStorage;
	class ServerBase;
	class SocketBase;

	void createServer();
}
}

class NodeRegistrator;
class NodeBase;

