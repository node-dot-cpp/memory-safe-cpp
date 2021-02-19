

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
		basic_string_literal( const char* str_)  {}
		basic_string_literal( const basic_string_literal& other ) = default;
		basic_string_literal& operator = ( const basic_string_literal& other ) = default;
		basic_string_literal( basic_string_literal&& other ) = default;
		basic_string_literal& operator = ( basic_string_literal&& other ) = default;

		bool operator == ( const basic_string_literal& other ) const;
		bool operator != ( const basic_string_literal& other ) const;

//		bool operator == ( const char* other ) const { return strcmp( str, other.str ) == 0; }
//		bool operator != ( const char* other ) const { return strcmp( str, other.str ) != 0; }

		const char* c_str() const;
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

	struct node_const_iterator
	{
	public:
		typedef node_const_iterator this_type;
		typedef int* pointer;
		typedef int& reference;
		node_const_iterator() { }

		node_const_iterator(const node_const_iterator&) = default;
		node_const_iterator& operator=(const node_const_iterator&) = default;
		node_const_iterator(node_const_iterator&&) = default;
		node_const_iterator& operator=(node_const_iterator&&) = default;

		reference operator*() const;
		pointer operator->() const;
		node_const_iterator& operator++();
		node_const_iterator operator++(int);
		bool operator==(const node_const_iterator& other) const;
		bool operator!=(const node_const_iterator& other) const;
	};

	struct node_iterator 
	{
	public:
		typedef node_iterator  this_type;
		typedef int* pointer;
		typedef int& reference;

		reference operator*() const;
		pointer operator->() const;
		this_type& operator++();
		bool operator==(const node_iterator&);
		bool operator!=(const node_iterator&);
	}; // node_iterator


	struct hashtable_const_iterator
	{
		typedef hashtable_const_iterator  this_type;
		typedef int* pointer;
		typedef int& reference;

		hashtable_const_iterator() { }

		hashtable_const_iterator(const hashtable_const_iterator& x) = default;
		hashtable_const_iterator& operator=(const hashtable_const_iterator& x) = default;
		hashtable_const_iterator(hashtable_const_iterator&&) = default;
		hashtable_const_iterator& operator=(hashtable_const_iterator&&) = default;
	public:
		reference operator*() const;
		pointer operator->() const;
		this_type& operator++();
		this_type operator++(int);
		bool operator==(const this_type& other) const;
		bool operator!=(const this_type& other) const;
		
	}; // hashtable_const_iterator


	struct hashtable_iterator
	{
	public:
		typedef hashtable_iterator this_type;
		typedef int* pointer;
		typedef int& reference;

	public:
		hashtable_iterator& operator=(const hashtable_iterator& x);
		reference operator*() const;
		pointer operator->() const;
		hashtable_iterator& operator++();
		bool operator==(const hashtable_iterator&);
		bool operator!=(const hashtable_iterator&);
	}; // hashtable_iterator


	class hash_map
	{
	public:
		typedef hash_map this_type;
		typedef int key_type;
		typedef int& reference;
		typedef int size_type;
		typedef int local_iterator;
		typedef int const_local_iterator;
		typedef int iterator;
		typedef int const_iterator;

		this_type& operator=(const this_type& x);
		void swap(this_type& x);

		reference       operator[](size_type n);
		reference       at(size_type n);

		iterator begin() ;
		const_iterator cbegin() const ;
		iterator end() ;
		const_iterator cend() const ;
		local_iterator begin(size_type n) ;
		const_local_iterator cbegin(size_type n) const ;
		local_iterator end(size_type) ;
		const_local_iterator cend(size_type) const ;
		bool empty() const ;
		size_type size() const ;
		size_type bucket_count() const ;
		size_type bucket_size(size_type n) const ;
		float load_factor() const ;

		void emplace();
		void emplace_hint();
		void try_emplace();
		void insert();
		void insert_or_assign();
		iterator         erase(const_iterator position);
		void clear();
		void rehash(size_type nBucketCount);
		void reserve(size_type nElementCount);

		iterator       find(const key_type& key);
		size_type count(const key_type& k) const ;

		void equal_range();

		// bool validate() const;
	};



	class safe_iterator_no_checks {
public:
	typedef int                 difference_type;
	typedef int*   				pointer;
	typedef int&	 				reference;

	safe_iterator_no_checks& operator=(const safe_iterator_no_checks& ri);

	reference operator*() const;

	pointer operator->() const;
	safe_iterator_no_checks& operator++();
	safe_iterator_no_checks& operator--();
	safe_iterator_no_checks operator+(difference_type n) const;
	safe_iterator_no_checks& operator+=(difference_type n);
	safe_iterator_no_checks operator-(difference_type n) const;
	safe_iterator_no_checks& operator-=(difference_type n);
	reference operator[](difference_type n) const;
	bool operator==(const safe_iterator_no_checks& ri) const;
	bool operator!=(const safe_iterator_no_checks& ri) const;
	bool operator<(const safe_iterator_no_checks& ri) const;
	bool operator>(const safe_iterator_no_checks& ri) const;
	bool operator<=(const safe_iterator_no_checks& ri) const;
	bool operator>=(const safe_iterator_no_checks& ri) const;

	};

	void distance(int, int);

	class safe_iterator_impl {
public:
	typedef int                 difference_type;
	typedef int*   				pointer;
	typedef int&	 				reference;

	safe_iterator_impl& operator=(const safe_iterator_impl& ri);

	reference operator*() const;

	pointer operator->() const;
	safe_iterator_impl& operator++();
	safe_iterator_impl& operator--();
	safe_iterator_impl operator+(difference_type n) const;
	safe_iterator_impl& operator+=(difference_type n);
	safe_iterator_impl operator-(difference_type n) const;
	safe_iterator_impl& operator-=(difference_type n);
	reference operator[](difference_type n) const;
	bool operator==(const safe_iterator_impl& ri) const;
	bool operator!=(const safe_iterator_impl& ri) const;
	bool operator<(const safe_iterator_impl& ri) const;
	bool operator>(const safe_iterator_impl& ri) const;
	bool operator<=(const safe_iterator_impl& ri) const;
	bool operator>=(const safe_iterator_impl& ri) const;

	};
} //namespace detail

} //namespace safememory


