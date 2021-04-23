
EASTL and containers
====================

This document is a short guide on how `EASTL` containers where _adapted_ into __safe__ containers.

Glosary for this doc:
* invalid memory: memory that has not been allocated, or worse, that belongs to someone else.
* zombie instance: an object instance whose destructor has already been called.
* zeroed instance: an object instance whose memory layout has been zeroed.
* static checker: companion tool to this library that will make checks on the code. 

Implementation of safe containers
---------------------------------

Safety on containers goes around 3 items:

1. Validation of method arguments.
2. Safety of iterators.
3. Safety of zombie and zeroed instances.


To get this working, we modified `eastl` containers as little as possible and implemented a safety layer over them, inheriting privately and passing a very custom allocator that is explained in detail below:

	namespace safememory
	{
		template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
		class vector : protected eastl::vector<T, detail::allocator_to_eastl_vector<Safety>>

### Validation of method arguments

We do it before calling the base container corresponding method:

	reference operator[](size_type n)
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException();
		}

		return base_type::operator[](n);
	}


### Safety of iterators

We define two kind of iterators for each container: _regular_ iterator and __safe__ iterator. None of them will dereference invalid memory. However both of them can dereference an _empty slot_, see [dezombiefy iterators](dezombiefy-iterators.md) for more information on this.

1. _Regular_ iterator. Has the same set of rules than a __raw__ pointer, should be allowed to exist only on the stack and lifetime should be validated by _static checker_.

2. __Safe__ iterator. Has the same set of rules that a `soft_ptr`, can be stored on the heap, and has means to verify where the target memory is still valid or not.

Each container provides both iterators types, and two sets of methods to work with one or the other kind:

	iterator       		begin() noexcept;
	iterator_safe       begin_safe() noexcept;


#### _Regular_ iterators
Since they don't allow to dereference any invalid memory, they can't be implemented with raw pointers. They are a full class that knows the valid iterable range and its current position, quite straight forward to implement.
Only to understand that each time an iterator comes in as argument, it has to be validated, converted to raw pointer, feed to the underlying `eastl` container, and then the returning raw pointer converted back to a _regular_ iterator:

	iterator insert(const_iterator_arg position, const value_type& value) {
		return makeIt(base_type::insert(toBase(position), value));
	}

#### __Safe__ iterators
They must merge the properties of _regular_ iterators with the properties of a `soft_ptr`, so so we must first a quick intro to `soft_ptr`.

Under `safememory` when an object is allocated on the heap, an special allocation function (`make_owning`) will allocate some extra memory to place a `ControlBlock` before the object, and an `owning_ptr` to such object will be returned. Then the `owning_ptr` knows about that `ControlBlock` and allows to create `soft_ptr` that _hook_ on the `ControlBlock`. When the `owning_ptr` is called to destroy the object, it releases its memory and notifies all `soft_ptr` that are still hooked.

The tricky part is how to get that `soft_ptr`, and how to make it hook to the `ControlBlock` without breaking everything in the process.
First attempt was to put `owning_ptr` inside the underlying containers, but that prove to be too complex, as ownership semantics of `owning_ptr` forced to a complete rewrite of most methods, droping all the adventages of using an underlying container.

Tried again, this time using __custom__ allocation function that makes the same as `make_owning` does, allocating some extra room for `ControlBlock` and initializing it, but it doesn't construct the object, and instead of returning an `owning_ptr`, it returns a `soft_ptr_with_zero_offset`. Then `soft_ptr` can be constructed form `soft_ptr_with_zero_offset` as it knows if there is a `ControlBlock` to hook, but unlike `owning_ptr` it does behave more like a raw pointer to minimize changes needed.

We modified `eastl` containers to use `soft_ptr_with_zero_offset` to hold the allocated pointers, and allocate them throught our __custom__ allocation functions. Then we can create `soft_ptr` from them and __safe__ iterators became a reality.


### Safety of zombie and zeroed instances

Types must remain safe even after destructor has run or even if we access a zeroed instance.
For zombie instances is usually enought to modifty the class destructor to put the instance in a valid state.
For zeored instances sometimes we are lucky enought that a zeroed instance is bit to bit identical to a valid state, but when that is not the case (like `unordered_map`) we must take extra precautions to verify we are not accessing a zeroed instance.



Container paricularities
------------------------

### safememory::vector
This is the most straight forward of all containers. Only particularity is that has __experimental__ support for `soft_this_ptr` on the elements it contains.
Vector allocates memory when the first element is pushed, so iterators to default constructed vector have `nullptr` inside.

### safememory::string
Underlying `eastl::basic_string` implements SSO (short string optimization), this means that when the string is short enought characters are stored inside the instance body and not on the heap. This is done internaly using an `union`.
For _regular_ iterators this works the same, but for __safe__ iterators, data has to be moved to the heap before.
Also a particularity of `eastl::basic_string` is that even default constructed instances have one null `'\0'` character in the buffer, so iterators to default constructed string don't have `nullptr` inside.
A zeroed `eastl::basic_string` is in an strange but valid state, of having 15 `'\0'` characters.

### safememory::unordered_map
Here `eastl::hashtable` has a couple of tricks we must address.

First on the table itself, the last pointer is asigned with a non-null but non dereferenceable value:

		void increment_bucket()
		{
			++mpBucket;
			while(*mpBucket == NULL) // We store an extra bucket with some non-NULL value at the end 
				++mpBucket;          // of the bucket array so that finding the end of the bucket
			mpNode = *mpBucket;      // array is quick and simple.
		}

So `soft_ptr_with_zero_offset` has to live with that, but such value can't propagate into `soft_ptr`.

Second, to avoid allocate on default construct, all instances of `eastl::hashtable` share a common static constant representation of an empty hash table. Only when the first element is inserted, allocation takes place. This is another _special_ value that `soft_ptr_with_zero_offset` has to live with, but can't propagate into `soft_ptr`.

Third, also because of point 2, a _zeroed_ instance of `eastl::hashtable` is in an invalid (dangerous) state. So before any access to the underlying `eastl::hashtable` we must verify it is in a valid state.


### safememory::array
Array does not use allocation, all elements are stored in the body of the array.
If array is created on the stack, all elements are on the stack. If we allocate an array on the heap, we are doing the allocation. Array internally never allocates, doesn't have an allocator, or does anything with memory. 

For _regular_ iterators this is not an issue, but for __safe__ iterators it is, as we can only create them when the array is on the heap. And to reach the `ControlBlock` we can only rely on constructors and some mechanims like `soft_this_ptr` does.

Now `easlt::array` uses aggreagate initialization:

		public:

		// Note that the member data is intentionally public.
		// This allows for aggregate initialization of the
		// object (e.g. array<int, 5> a = { 0, 3, 2, 4 }; )
		value_type mValue[N ? N : 1];


But such has very narrow C++ rules requiring no user constructor and no members with user constructors. So we must either drop `eastl::array` or drop __safe__ iterators.
The result is a fully custom implementation of `safememory::array` not depending on any underlying implementation, and using a constructor with `std::initializer_list<T>`.
This implementation can't be used in `constexpr` context. And may have other issues I can't foresee at this time.


### safememory::basic_string_literal
String literal class don't exist on `std` or `eastl` so is fully implemented on `safememory`.
The important part is that while we can't create `soft_ptr` because literal has no `ControlBlock`, a _regular_ iterator would be __safe__ because literal will live in memory forever. We only need _static checker_ to understand this diference.


Allocator and dependency order
------------------------------
Since `safememory` library depends (or uses) `eastl` containers, that stablishes a dependency order.
If from `eastl` we try to `#include` something from `safememory` we would be inverting the dependecy order and that would cause `#include` loops (an is very bad practice).
Then all types and functions from `safememory` that `eastl` containers use must be _injected_, and since adding one more template parameter everywhere whould have required a lot of _intrussion_ at `eastl` we decided to overload the existing __Allocator__ template parameter, to fullfil all the required tasks.
We know that adding more than one responsability to a single template parameter is also bad practice, but in this particular case we felt it gives the best balance.


Allocator responsabilities
--------------------------
### Safety safe/none

On `safememory` all pointer wrappers support the idea of compile time safety on/off, this is achived throught template parameter enum `memory_safety::none` and `memory_safety::safe`. This idea is forwarded into `eastl` containers throught the allocator type. We use one allocator type when `memory_safety::none` and a different allocator type when `memory_safety::safe`:


    template<memory_safety Safety>
    using allocator_to_eastl_vector = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_vector_impl, allocator_to_eastl_vector_no_checks>;


### Allocated pointers aliases

On `safememory` we use wrappers for allocated heap memory pointers. More, we make a distiction between a pointer to an object and a pointer to an array of objects (i.e. the later allows `operator[]` while the first not). Allocator provides both type aliases for containers to know about:

    template<class T>
	using pointer = soft_ptr_with_zero_offset_impl<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_impl<flexible_array<T>>;


### Allocate / deallocate

Allocator of course provides means to allocate / deallocate one object, and also an array of objects:

	template<class T>
	pointer<T> allocate_node();

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0);

	template<class T>
	void deallocate_node(const pointer<T>& p);

	template<class T>
	void deallocate_array(const array_pointer<T>& p, std::size_t count);

### Allocated pointer conversion

Also provides means to convert allocated wrappers to __raw__ pointers and to `soft_ptr`:

	template<class T>
	static soft_ptr_impl<T> to_soft(const pointer<T>& p);

	template<class T>
	static T* to_raw(const pointer<T>& p);

### Special values

Has knowledge of two _special_ pointer values used by `eastl::hashtable`, that should be supported inside `soft_ptr_with_zero_offset` but can't be mapped to `soft_ptr` because there is no `ControlBlock` to hook.


	template<class T>
	static pointer<T> get_hashtable_sentinel();

	template<class T>
	static array_pointer<T> get_empty_hashtable();


### Helper for `soft_this_ptr`

And last, provides a __RAII__ helper to allow `soft_this_ptr` to work when an element is pushed by-value inside an `EASTL::vector`:

	template<class T>
	static soft_this_ptr_raii<T> make_raii(const pointer<T>& p);


Important files
---------------

### `safememory/detail/allocator_to_eastl.h`

Here is where all the magic is hidden (and most likely all the bugs).
The most critical part are allocation functions at the beginning of the file, the bottom 3 are the entry points, and the top 2 are called internally from the others:

	template<std::size_t alignment>
	void* zombie_allocate_helper(std::size_t sz);

	template<memory_safety is_safe, std::size_t alignment>
	std::pair<make_zero_offset_t, void*> allocate_helper(std::size_t sz);

	template<memory_safety is_safe, class T>
	void deallocate_helper(const soft_ptr_with_zero_offset<T, is_safe>& p);

	template<memory_safety is_safe, class T>
	soft_ptr_with_zero_offset<T, is_safe> allocate_node_helper();

	template<memory_safety is_safe, typename T, bool zeroed>
	soft_ptr_with_zero_offset<flexible_array<T>, is_safe> allocate_flexible_array_helper(std::size_t count);


### `safememory/detail/soft_ptr_with_zero_offset.h`
All container alocations return an instance of `soft_ptr_with_zero_offset`, and the main function of this class is to be a _marker_ of such thing. There are 4 specializations of this class to match each of the 4 allocators functions described above.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct the instance it points.
* They don't own the memory, can be copied.
* They can be used inside `union` (`eastl::string` needs that).
* They can have _special_ values that point to static or invalid memory (`eastl::hashtable` needs that).
* Allocator can create a `soft_ptr` from them, but first _special_ values are checked.


### `safememory/detail/flexible_array.h`
All array allocations return an instance of `soft_ptr_with_zero_offset<flexible_array<T>>`, and the main function is to be a marker of the existance of an array in the memory layout.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct any instance inside such memory array.
* Specialized `soft_ptr_with_zero_offset<flexible_array<T>>` has array operators overloaded and pointer arithmetics, so simplify changes.

