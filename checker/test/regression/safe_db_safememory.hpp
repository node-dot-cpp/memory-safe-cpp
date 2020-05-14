

/*
 * This file contain type names and functions names that are declared as 'safe'
 * under Rule S8.
 * At this moment only names are of interest, we don't care about arguments,
 * return type or template parameters. This simplifies this file a lot.
 * 
 */


namespace safememory {

	struct hash
	{
		long operator()() const;
	};

	// since we don't support overloads here, these operators are valids
	// for any type defined in the safememory namespace
	bool operator==(hash, hash);
	bool operator!=(hash, hash);
	bool operator<(hash, hash);
	bool operator<=(hash, hash);
	bool operator>(hash, hash);
	bool operator>=(hash, hash);
	void swap(hash, hash);


	class basic_string
	{
	public:
		typedef basic_string         this_type;
		typedef int          		size_type;
		typedef int 				value_type;
		typedef char&                reference;
		typedef const char&          const_reference;
		typedef int					 iterator;
		typedef int					 const_iterator;
		typedef int                	 reverse_iterator;
		typedef int          	     const_reverse_iterator;

		// Operator=
		this_type& operator=(const this_type& x);
		void swap(this_type& x) ;

		// Assignment operations
		this_type& assign(const this_type& x);


		this_type& assign_convert();


		iterator       begin() ;
		const_iterator cbegin() const ;

		iterator       end() ;
		const_iterator cend() const ;

		reverse_iterator       rbegin() ;
		const_reverse_iterator crbegin() const ;

		reverse_iterator       rend() ;
		const_reverse_iterator crend() const ;


		// Size-related functionality
		bool      empty() const ;
		size_type size() const ;
		size_type length() const ;
		size_type max_size() const ;
		size_type capacity() const ;
		void      resize(size_type n);
		void      reserve(size_type = 0);
		void      force_size(size_type n);
		void      set_capacity(size_type n);
		void 	  shrink_to_fit();

		// Element access
		reference       operator[](size_type n);
		reference       at(size_type n);
		reference       front();
		reference       back();

		// Append operations
		this_type& operator+=(const this_type& x);

		this_type& append(const this_type& x);

		this_type& append_convert();

		void push_back(value_type c);
		void pop_back();

		this_type& insert(size_type position, const this_type& x);
		this_type&       erase(size_type position = 0, size_type n);
		void             clear() ;
		this_type&  replace(size_type position, size_type n,  const this_type& x);
		size_type find(const this_type& x,  size_type position = 0) const ;
		size_type rfind(const this_type& x,  size_type position) const ;
		size_type find_first_of(const this_type& x, size_type position = 0) const ;
		size_type find_last_of(const this_type& x, size_type position = 0) const ;
		size_type find_first_not_of(const this_type& x, size_type position = 0) const ;
		size_type find_last_not_of(const this_type& x,  size_type position = 0) const ;
		this_type substr(size_type position = 0, size_type n = 0) const;
		int        compare(const this_type& x) const ;

		// Misc functionality, not part of C++ this_type.
		void         make_lower();
		void         make_upper();
		void         ltrim();
		void         rtrim();
		void         trim();
		this_type    left(size_type n) const;
		this_type    right(size_type n) const;

		size_t hash() const ;

		// bool validate() const ;


	}; // basic_string

	class string_literal {
		void operator=(const string_literal&);
	};


	class vector
	{
		typedef vector                          this_type;

	public:
		typedef int								      size_type;
		typedef int                                             value_type;
		typedef int*                                            pointer;
		typedef const int*                                      const_pointer;
		typedef int&                                            reference;
		typedef const int&                                      const_reference;

		typedef int							iterator_safe;
		typedef int						const_iterator_safe;
		typedef int                reverse_iterator_safe;
		typedef int          const_reverse_iterator_safe;
		typedef const const_iterator_safe&							csafe_it_arg;

		typedef int                                       iterator;
		typedef int                                 const_iterator;
		typedef int             					reverse_iterator;
		typedef int							const_reverse_iterator;    

		this_type& operator=(const this_type& x);
		void swap(this_type& x);
		void assign(size_type n, const value_type& value);

		iterator_safe       begin() ;
		const_iterator_safe cbegin() const ;

		iterator_safe       end() ;
		const_iterator_safe cend() const ;

		reverse_iterator_safe       rbegin() ;
		const_reverse_iterator_safe crbegin() const ;

		reverse_iterator_safe       rend() ;
		const_reverse_iterator_safe crend() const ;

		bool      empty() const ;
		size_type size() const ;
		size_type capacity() const ;
		size_type max_size() const ;

		void resize(size_type n, const value_type& value);
		void reserve(size_type n);
		void shrink_to_fit();

		reference       operator[](size_type n);
		reference       at(size_type n);
		reference       front();
		reference       back();

		void      push_back(const value_type& value);
		void      pop_back();

		iterator_safe emplace();

		reference emplace_back();

		iterator_safe insert(csafe_it_arg position, const value_type& value);
		iterator_safe erase(csafe_it_arg position);
		void clear() ;
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
	}; // node_iterator

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


namespace detail {
	class unsafe_iterator {
public:
	typedef int                 difference_type;
	typedef int*   				pointer;
	typedef int&	 				reference;

	unsafe_iterator& operator=(const unsafe_iterator& ri);

	reference operator*() const;

	pointer operator->() const;
	unsafe_iterator& operator++();
	unsafe_iterator& operator--();
	unsafe_iterator operator+(difference_type n) const;
	unsafe_iterator& operator+=(difference_type n);
	unsafe_iterator operator-(difference_type n) const;
	unsafe_iterator& operator-=(difference_type n);
	reference operator[](difference_type n) const;
	bool operator==(const unsafe_iterator& ri) const;
	bool operator!=(const unsafe_iterator& ri) const;
	bool operator<(const unsafe_iterator& ri) const;
	bool operator>(const unsafe_iterator& ri) const;
	bool operator<=(const unsafe_iterator& ri) const;
	bool operator>=(const unsafe_iterator& ri) const;

	};

	void distance(int, int);

	class safe_iterator {
public:
	typedef int                 difference_type;
	typedef int*   				pointer;
	typedef int&	 				reference;

	safe_iterator& operator=(const safe_iterator& ri);

	reference operator*() const;

	pointer operator->() const;
	safe_iterator& operator++();
	safe_iterator& operator--();
	safe_iterator operator+(difference_type n) const;
	safe_iterator& operator+=(difference_type n);
	safe_iterator operator-(difference_type n) const;
	safe_iterator& operator-=(difference_type n);
	reference operator[](difference_type n) const;
	bool operator==(const safe_iterator& ri) const;
	bool operator!=(const safe_iterator& ri) const;
	bool operator<(const safe_iterator& ri) const;
	bool operator>(const safe_iterator& ri) const;
	bool operator<=(const safe_iterator& ri) const;
	bool operator>=(const safe_iterator& ri) const;

	};
} //namespace detail

} //namespace safememory

