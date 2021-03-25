
EASTL and containers
====================

This document is a short guide on how `EASTL` containers where _adapted_ into __safe__ containers.

Safety on containers
--------------------
Safety on containers is mostly around iterators: __safe__ iterators and _regular_ iterators.
Under `safememory` context, a _regular_ iterator is allowed to exist only on the stack, should be enforces lifetime checks, but it is not allowed to dereference any invalid memory. We can say that it has the same set of rules that a raw pointer.
A __safe__ iterator on the other hand, has the same set of rules that a `soft_ptr`, can be stored on the heap, and has means to verify where the target memory is still valid or not.

Each container provides both iterators types, and two sets of methods to work with one or the other kind:

	iterator       		begin() noexcept;
	iterator_safe       begin_safe() noexcept;



Also each safe container has also an __\_safe__ sister (i.e. `vector_safe`, `string_safe`, etc) where all methods and iterators are safe. i.e. `vector_safe::iterator` => `vector::iterator_safe`, `vector_safe::begin()` => `vector::begin_safe()`, and so on.



Implementation of safe containers
---------------------------------
To get this working, we modified `eastl` containers as little as possible and implemented a safety layer over them, inheriting privately and passing a very custom allocator that is explained in detail below:

	namespace safememory
	{
		template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
		class vector : protected eastl::vector<T, detail::allocator_to_eastl_vector<Safety>>


Some methods require validation of its arguments to be safe, we do that before calling the base container corresponding method:

	reference operator[](size_type n)
	{
		if constexpr(is_safe == memory_safety::safe) {
			if(NODECPP_UNLIKELY(n >= size()))
				ThrowRangeException("vector::operator[] -- out of range");
		}

		return base_type::operator[](n);
	}

For _regular_ iterators, since they don't allow to dereference any invalid memory, they can't be implemented with raw pointers. They are a full class that knows the valid iterable range and its current position, quite straight forward and nothing obscure here.
Only to understand that each time an iterator comes in as argument, it has to be converted to raw pointer, feed to the underlying `eastl` container, and then the returning raw pointer converted back to a _regular_ iterator:


	iterator insert(const_iterator_arg position, const value_type& value) {
		return makeIt(base_type::insert(toBase(position), value));
	}


For __safe__ iterators we must merge the properties of _regular_ iterators with the properties of a `soft_ptr`, so so we must first understand how `soft_ptr` works.

Under `safememory` when an object is allocated on the heap, an special allocation function (`make_owning`) will allocate some extra memory to place a `ControlBlock` before the object, and an `owning_ptr` to such object will be returned. Then the `owning_ptr` knows about that `ControlBlock` and allows to create `soft_ptr` that _hook_ on the `ControlBlock`. When the `owning_ptr` is called to destroy the object, it releases its memory and notifies all `soft_ptr` that are still hooked.

The tricky part is how to get that `soft_ptr`, and how to make it hook to the `ControlBlock` without breaking everything in the process.
First attempt was to put `owning_ptr` inside the underlying containers, but that prove to be too complex, as ownership semantics of `owning_ptr` forced to a complete rewrite of most methods, droping all the adventages of using an underlying container.

Tried again, this time using __custom__ allocation function that makes the same as `make_owning` does, allocating some extra room for `ControlBlock` and initializing it, but it doesn't construct the object, and instead of returning an `owning_ptr`, it returns a `soft_ptr_with_zero_offset`. Then `soft_ptr` can be constructed form `soft_ptr_with_zero_offset` as it knows if there is a `ControlBlock` to hook, but unlike `owning_ptr` it does behave more like a raw pointer to minimize changes needed.

We modified `eastl` containers to use `soft_ptr_with_zero_offset` to hold the allocated pointer, and allocate them throught our __custom__ allocation functions. Then we can create `soft_ptr` from them and __safe__ iterators became a reality.


Dependency order
----------------
Since `safememory` library depends (or uses) `eastl` containers, that stablishes a dependency order.
If from `eastl` we try to `#include` something from `safememory` we would be inverting the dependecy order and that would cause `#include` loops (an is very bad practice).
Then all types and functions from `safememory` that `eastl` containers use must be _injected_, and since adding one more template parameter everywhere whould have required a lot of _intrussion_ at `eastl` we decided to overload the existing __Allocator__ template parameter, to fullfil all the required tasks.
We know that adding more than one responsability to a single template parameter is also bad practice, but in this particular case we felt it gives the best balance.


Allocator responsabilities
--------------------------
On `safememory` all pointer wrappers support the idea of compile time safety on/off, this is achived throught template parameter enum `memory_safety::none` and `memory_safety::safe`. This idea is forwarded into `eastl` containers throught the allocator type. We use one allocator type when `memory_safety::none` and a different allocator type when `memory_safety::safe`:


    template<memory_safety Safety>
    using allocator_to_eastl_vector = std::conditional_t<Safety == memory_safety::safe,
			allocator_to_eastl_vector_impl, allocator_to_eastl_vector_no_checks>;



On `safememory` we use wrappers for allocated heap memory pointers. More, we make a distiction between a pointer to an object and a pointer to an array of objects (i.e. the later allows `operator[]` while the first not). Allocator provides both type aliases for containers to know about:

    template<class T>
	using pointer = soft_ptr_with_zero_offset_impl<T>;

	template<class T>
	using array_pointer = soft_ptr_with_zero_offset_impl<flexible_array<T>>;


Allocator of course provides means to allocate / deallocate one object, and also an array of objects:

	template<class T>
	pointer<T> allocate_node();

	template<class T>
	array_pointer<T> allocate_array(std::size_t count, int flags = 0);

	template<class T>
	void deallocate_node(const pointer<T>& p);

	template<class T>
	void deallocate_array(const array_pointer<T>& p, std::size_t count);


Also provides means to convert allocated wrappers to __raw__ pointers and to `soft_ptr`:

	template<class T>
	static soft_ptr_impl<T> to_soft(const pointer<T>& p);

	template<class T>
	static T* to_raw(const pointer<T>& p);


Has knowledge of two _special_ pointer values used by `eastl::hashtable`, that should be supported inside `soft_ptr_with_zero_offset` but can't be mapped to `soft_ptr` because there is no `ControlBlock` to hook.


	template<class T>
	static pointer<T> get_hashtable_sentinel();

	template<class T>
	static array_pointer<T> get_empty_hashtable();


And last, provides a __RAII__ helper to allow `soft_this_ptr` to work when an element is pushed by-value inside an `EASTL::vector`:

	template<class T>
	static soft_this_ptr_raii<T> make_raii(const pointer<T>& p);


Important files
---------------

### `safememory/detail/allocator_to_eastl.h`

Here is where all the magic is hidden (and most likely all the bugs).
The most critical part are 4 allocation functions at the beginning of the file:

    template<class T>
    soft_ptr_with_zero_offset_impl<T> allocate_impl();

    template<class T, bool zeroed>
    soft_ptr_with_zero_offset_impl<flexible_array<T>> allocate_array_impl(std::size_t count);

    template<class T>
    soft_ptr_with_zero_offset_no_checks<T> allocate_no_checks();

    template<class T, bool zeroed>
    soft_ptr_with_zero_offset_no_checks<flexible_array<T>> allocate_array_no_checks(std::size_t count);


### `safememory/detail/soft_ptr_with_zero_offset.h`
All container alocations return an instance of `soft_ptr_with_zero_offset`, and the main function of this class is to be a _marker_ of such thing. There are 4 specializations of this class to match each of the 4 allocators functions described above.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct the instance it points.
* They don't own the memory, can be copied.
* They can be used inside `union` (`eastl::string` needs that).
* They can have _special_ values that point to static or invalid memory (`eastl::hashtable` needs that).
* Allocator can create a `soft_ptr` from them, but first _special_ values are checked.
* All 

### `safememory/detail/flexible_array.h`
All array allocations return an instance of `soft_ptr_with_zero_offset<flexible_array<T>>`, and the main function is to be a marker of the existance of an array in the memory layout.

Some importan points:
* Only the allocator creates them.
* They won't construct or destruct any instance inside such memory array.
* Specialized `soft_ptr_with_zero_offset<flexible_array<T>>` has array operators overloaded and pointer arithmetics, so simplify changes.

