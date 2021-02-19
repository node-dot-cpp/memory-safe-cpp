

/*
 * This file contain type names and functions names that are declared as 'safe'
 * under Rule S8.
 * At this moment only names are of interest, we don't care about arguments,
 * return type or template parameters. This simplifies this file a lot.
 * 
 */

namespace std {
 	void move();
 	void forward();

	class function {
		void operator=(function&);
	};

	struct fake { int i = 0;};
	bool operator==(fake, fake);
	bool operator!=(fake, fake);
	bool operator<(fake, fake);
	bool operator<=(fake, fake);
	bool operator>(fake, fake);
	bool operator>=(fake, fake);
	void swap(fake, fake);

	// apple gcc std lib uses namespace __1
	namespace __1 {
		void move();
		void forward();
		class function {
			void operator=(function&);
		};

		struct fake { int i = 0;};
		bool operator==(fake, fake);
		bool operator!=(fake, fake);
		bool operator<(fake, fake);
		bool operator<=(fake, fake);
		bool operator>(fake, fake);
		bool operator>=(fake, fake);
		void swap(fake, fake);
	}

	namespace experimental {

		class suspend_never {
			void await_ready();
			void await_resume();
			void await_suspend();

		};

		class coroutine_handle {
			void from_address();
		};
		namespace coroutines_v1 {
			class coroutine_handle;
			void operator!=(void*, coroutine_handle);
		}
	}
}
