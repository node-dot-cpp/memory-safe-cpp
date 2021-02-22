

/*
 * This file contain type names and functions names that are declared as 'safe'
 * under Rule S8.
 * At this moment only names are of interest, we don't care about arguments,
 * return type or template parameters. This simplifies this file a lot.
 * 
 */


namespace safememory {

	struct fake { int i = 0;};
	
	// since we don't support overloads here, these operators are valids
	// for any type defined in the safememory namespace
	bool operator==(fake, fake);
	bool operator!=(fake, fake);
	bool operator<(fake, fake);
	bool operator<=(fake, fake);
	bool operator>(fake, fake);
	bool operator>=(fake, fake);
	void swap(fake, fake);

	void make_owning();
	void make_owning_2();

	namespace detail {
		class owning_ptr_impl {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			owning_ptr_impl& operator=(const owning_ptr_impl&);
			fake& get() const;
		};

		class owning_ptr_no_checks {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			owning_ptr_no_checks& operator=(const owning_ptr_no_checks&);
			fake& get() const;
		};

		class soft_ptr_impl {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			soft_ptr_impl& operator=(const soft_ptr_impl&);
			fake& get() const;
		};

		class soft_ptr_no_checks {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			soft_ptr_no_checks& operator=(const soft_ptr_no_checks&);
			fake& get() const;
		};

		class nullable_ptr_impl {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			nullable_ptr_impl& operator=(const nullable_ptr_impl&);
			fake& get() const;
		};

		class nullable_ptr_no_checks {
			operator bool();
			fake* operator->() const;
			fake& operator*() const;
			nullable_ptr_no_checks& operator=(const nullable_ptr_no_checks&);
			fake& get() const;
		};
	} //namespace detail

	struct hash
	{
		long operator()() const;
	};


	class basic_string_literal
	{
	public:
		typedef basic_string_literal this_type;

		this_type& operator=(const this_type& other);

		void begin();
		void cbegin();
		void end();
		void cend();
		void rbegin();
		void crbegin();
		void rend();
		void crend();

		void empty();
		void size();
		// void c_str();
		// void data();
		void operator[](int);
		void at();
		void front();
		void back();
	};


	class basic_string {
	public:
		typedef basic_string this_type;

		this_type& operator=(const this_type& x);

		void swap(this_type& x);

		void assign();
		// void assign_unsafe();

		void begin();
		void cbegin();
		void end();
		void cend();
		void rbegin();
		void crbegin();
		void rend();
		void crend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();
		void rbegin_safe();
		void crbegin_safe();
		void rend_safe();
		void crend_safe();

		void empty();
		void size();
		void length();
		void max_size();
		void capacity();
		void resize();
		void reserve();
		void force_size();
		void shrink_to_fit();

		// void data();
		// void c_str();

		void operator[](int);
		void at();
		void front();
		void back();

		this_type& operator+=(const this_type& x);
		void append();
		void append_unsafe();
		void append_convert();
		void append_convert_unsafe();
		void push_back();
		void pop_back();
		void insert();
		void insert_safe();
		void erase();
		void erase_safe();
		void clear();
		void replace();
		void replace_safe();



		void find();
		void rfind();
		void find_first_of();
		void find_last_of();
		void find_first_not_of();
		void find_last_not_of();

		void substr();
		void compare();
		void comparei();

		void make_lower();
		void make_upper();
		void ltrim();
		void rtrim();
		void trim();
		void left();
		void right();

		bool operator==(const this_type& b)	const;
		bool operator!=(const this_type& b) const;
		bool operator<(const this_type& b) const;
		bool operator>(const this_type& b) const;
		bool operator<=(const this_type& b) const;
		bool operator>=(const this_type& b) const;

		void validate();
		void validate_iterator();
		void make_safe();
	}; // basic_string


	class basic_string_safe {
	public:
		typedef basic_string_safe this_type;

		this_type& operator=(const this_type& x);

		void swap(this_type& x);

		void assign();
		// void assign_unsafe();

		void begin();
		void cbegin();
		void end();
		void cend();
		void rbegin();
		void crbegin();
		void rend();
		void crend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();
		void rbegin_safe();
		void crbegin_safe();
		void rend_safe();
		void crend_safe();

		void empty();
		void size();
		void length();
		void max_size();
		void capacity();
		void resize();
		void reserve();
		void force_size();
		void shrink_to_fit();

		// void data();
		// void c_str();

		void operator[](int);
		void at();
		void front();
		void back();

		this_type& operator+=(const this_type& x);
		void append();
		void append_unsafe();
		void append_convert();
		void append_convert_unsafe();
		void push_back();
		void pop_back();
		void insert();
		void insert_safe();
		void erase();
		void erase_safe();
		void clear();
		void replace();
		void replace_safe();



		void find();
		void rfind();
		void find_first_of();
		void find_last_of();
		void find_first_not_of();
		void find_last_not_of();

		void substr();
		void compare();
		void comparei();

		void make_lower();
		void make_upper();
		void ltrim();
		void rtrim();
		void trim();
		void left();
		void right();

		bool operator==(const this_type& b)	const;
		bool operator!=(const this_type& b) const;
		bool operator<(const this_type& b) const;
		bool operator>(const this_type& b) const;
		bool operator<=(const this_type& b) const;
		bool operator>=(const this_type& b) const;

		void validate();
		void validate_iterator();
		void make_safe();
	}; // basic_string_safe

	class vector {
	public:
		typedef vector this_type;
		this_type& operator=(const this_type& x);

		void swap(this_type& x) noexcept;

		void assign();
		// void assign_unsafe();

		void begin();
		void cbegin();
		void end();
		void cend();
		void rbegin();
		void crbegin();
		void rend();
		void crend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();
		void rbegin_safe();
		void crbegin_safe();
		void rend_safe();
		void crend_safe();

		void empty();
		void size();
		void capacity();
		void max_size();
		void resize();
		void reserve();
		void set_capacity();
		void shrink_to_fit();

		// void data_unsafe();
		void operator[](int);
		void at();
		void front();
		void back();
		void push_back();
		void pop_back();

		// void emplace_unsafe();
		void emplace();
		void emplace_safe();
		void emplace_back();

		// void insert_unsafe();
		void insert();
		void insert_safe();

		// void erase_unsafe();
		// void erase_unsorted_unsafe();
		void erase();
		void erase_unsorted();
		void erase_safe();
		void erase_unsorted_safe();

		void clear();

		void validate();
		void validate_iterator();
		void make_safe();
	}; // class vector

	class vector_safe {
	public:
		typedef vector_safe this_type;
		this_type& operator=(const this_type& x);

		void swap(this_type& x) noexcept;

		void assign();
		// void assign_unsafe();

		void begin();
		void cbegin();
		void end();
		void cend();
		void rbegin();
		void crbegin();
		void rend();
		void crend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();
		void rbegin_safe();
		void crbegin_safe();
		void rend_safe();
		void crend_safe();

		void empty();
		void size();
		void capacity();
		void max_size();
		void resize();
		void reserve();
		void set_capacity();
		void shrink_to_fit();

		// void data_unsafe();
		void operator[](int);
		void at();
		void front();
		void back();
		void push_back();
		void pop_back();

		// void emplace_unsafe();
		void emplace();
		void emplace_safe();
		void emplace_back();

		// void insert_unsafe();
		void insert();
		void insert_safe();

		// void erase_unsafe();
		// void erase_unsorted_unsafe();
		void erase();
		void erase_unsorted();
		void erase_safe();
		void erase_unsorted_safe();

		void clear();

		void validate();
		void validate_iterator();
		void make_safe();
	}; // class vector_safe

	class unordered_map {
	public:
		typedef unordered_map this_type;

	public:
		this_type& operator=(const this_type& x);
		void swap(this_type& x);

		void begin();
		void cbegin();
		void end();
		void cend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();

		void at();
		void operator[](int);

		void empty();
		void size();
		void bucket_count();
		void bucket_size();

		void load_factor();
		void get_max_load_factor();
		void set_max_load_factor();
		void rehash_policy();

		void emplace();
		void emplace_safe();
		void emplace_hint();
		void emplace_hint_safe();
		void try_emplace();
		void try_emplace_safe();
		void insert();
		void insert_safe();
		// void insert_unsafe();

		void insert_or_assign();
		void insert_or_assign_safe();
		void erase();
		void erase_safe();

		void clear();
		void rehash();
		void reserve();
		void find();
		void find_safe();
		void count();

		void equal_range();
		void equal_range_safe();
		bool operator==(const this_type& other) const;
		bool operator!=(const this_type& other) const;

		void validate();
		void validate_iterator();

		void make_safe();
	}; // unordered_map

	class unordered_map_safe {
	public:
		typedef unordered_map_safe this_type;

	public:
		this_type& operator=(const this_type& x);
		void swap(this_type& x);

		void begin();
		void cbegin();
		void end();
		void cend();

		void begin_safe();
		void cbegin_safe();
		void end_safe();
		void cend_safe();

		void at();
		void operator[](int);

		void empty();
		void size();
		void bucket_count();
		void bucket_size();

		void load_factor();
		void get_max_load_factor();
		void set_max_load_factor();
		void rehash_policy();

		void emplace();
		void emplace_safe();
		void emplace_hint();
		void emplace_hint_safe();
		void try_emplace();
		void try_emplace_safe();
		void insert();
		void insert_safe();
		// void insert_unsafe();

		void insert_or_assign();
		void insert_or_assign_safe();
		void erase();
		void erase_safe();

		void clear();
		void rehash();
		void reserve();
		void find();
		void find_safe();
		void count();

		void equal_range();
		void equal_range_safe();
		bool operator==(const this_type& other) const;
		bool operator!=(const this_type& other) const;

		void validate();
		void validate_iterator();

		void make_safe();
	}; // unordered_map_safe




	namespace detail {

		class hashtable_heap_safe_iterator {
		public:
			typedef hashtable_heap_safe_iterator this_type;

			this_type& operator=(const this_type&);

			int& operator*() const;
			int* operator->() const;
			this_type& operator++();

			bool operator==(const this_type& other) const;
			bool operator!=(const this_type& other) const;
		}; // hashtable_heap_safe_iterator

		class hashtable_stack_only_iterator
		{
		public:
			typedef hashtable_stack_only_iterator this_type;

			this_type& operator=(const this_type& ri);

			int& operator*() const;
			int* operator->() const;
			this_type& operator++();

			bool operator==(const this_type& other) const;
			bool operator!=(const this_type& other) const;
		}; // hashtable_stack_only_iterator


		class array_of_iterator
		{
		public:
			typedef array_of_iterator this_type;
			this_type& operator=(const this_type& ri);

			int& operator*() const;
			int* operator->() const;

			this_type& operator++() noexcept;
			this_type& operator--() noexcept;

			this_type operator+(int) const noexcept;
			this_type operator-(int) const noexcept;

			this_type& operator+=(int) noexcept;
			this_type& operator-=(int) noexcept;

			constexpr int& operator[](int) const;
			int operator-(const this_type& ri) const noexcept;

			bool operator==(const this_type& ri) const noexcept;
			bool operator!=(const this_type& ri) const noexcept;
			bool operator<(const this_type& ri) const noexcept;
			bool operator>(const this_type& ri) const noexcept;
			bool operator<=(const this_type& ri) const noexcept;
			bool operator>=(const this_type& ri) const noexcept;
		};

		int distance(const array_of_iterator&, const array_of_iterator&);
	} //namespace detail

} //namespace safememory



namespace eastl {
	// some methods are used directly from eastl base classes
	// so we must white-list them here

	struct fake { int i = 0;};
	
	// since we don't support overloads here, these operators are valids
	// for any type defined in the eastl namespace                                                                                 
	bool operator==(fake, fake);
	bool operator!=(fake, fake);
	bool operator<(fake, fake);
	bool operator<=(fake, fake);
	bool operator>(fake, fake);
	bool operator>=(fake, fake);
	void swap(fake, fake);

	class basic_string {
	public:
		void empty();
		void size();
		void length();
		void max_size();
		void capacity();
		void resize();
		void reserve();
		void force_size();
		void shrink_to_fit();

		void front();
		void back();

		void push_back();
		void clear();

		void make_lower();
		void make_upper();

		void validate();
	}; // basic_string


	class vector {
	public:

		void empty();
		void size();
		void capacity();
		// void max_size();
		void resize();
		void reserve();
		void set_capacity();
		void shrink_to_fit();

		void push_back();
		void emplace_back();

		void clear();

		void validate();
	}; // class vector


	struct node_iterator
	{
	public:
		typedef node_iterator this_type;

		this_type& operator=(const this_type& ri);

		int& operator*() const;
		int* operator->() const;
		this_type& operator++();
	}; // node_iterator

	struct hashtable_iterator
	{
	public:
		typedef hashtable_iterator this_type;

		this_type& operator=(const this_type& ri);

		int& operator*() const;
		int* operator->() const;
		this_type& operator++();
	}; // hashtable_iterator

	class hashtable {
	public:
		void empty();
		void size();
		void bucket_count();
		void bucket_size();

		void load_factor();
		void get_max_load_factor();
		void set_max_load_factor();
		void rehash_policy();

		void clear();
		void rehash();
		void reserve();
		void count();
		void validate();
	};

	class hash_map {
	public:
		void at();
		void operator[](int);
	};

}

