

/*
 * This file contain type names and functions names that are declared as 'safe'
 * under Rule S8.
 * At this moment only names are of interest, we don't care about arguments,
 * return type or template parameters. This simplifies this file a lot.
 * 
 */

#include "safe_db_std.hpp"
#include "safe_db_safe_memory.hpp"
#include "safe_db_nodecpp.hpp"

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

namespace safe_memory {
	void make_owning();
	void soft_ptr_static_cast();
	// osn ptrs are hardcoded with special safety rules
	// class owning_ptr;
	// class soft_ptr;
	// class nullable_ptr;
}

namespace nodecpp {
	void isException();
	void getException();
	void setException();

	void setNoException();
	void setTimeoutForAction();
	void clearTimeout();

	class DataParent;
	class map {
		void insert();
		void erase();
		void begin();
		void end();
		void clear();
		void size();
	};

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

void getArgv();
class NodeRegistrator;
class NodeBase;

