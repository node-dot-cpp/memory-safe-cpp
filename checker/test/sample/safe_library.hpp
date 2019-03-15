
namespace fmt {
namespace v5 {
	void print();
}	
}

namespace std {
	void move();
	void forward();
}

namespace nodecpp {
namespace safememory {
	void make_owning();

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
}
}

class NodeRegistrator;
class NodeBase;

