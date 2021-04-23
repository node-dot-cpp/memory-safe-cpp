
Dezombiefy iterators
====================

Glosary for this doc:
* invalid memory: memory that has not been allocated, or worse, that belongs to someone else.
* zombie instance: an object instance whose destructor has already been called.
* zeroed instance: an object instance whose memory layout has been zeroed.


The problem described here afect iterators of `vector` and `string`, but analisys focus on `vector` because accessing a zombie complex type inside a `vector` adds a little extra complexity than accessing just characters in `string`.

`vector` has a __heap buffer__ with _empty slots_ where elements are placed when we `push_back`, since elements can be added or removed after iterator is created (more the `std` says that `push_back` and `pop_back` don't invalidate iterators unless there is __heap buffer__ reallocation, and algorithms can use this by calling `reserve` earlier), the iterator doesn't now the exact number of valid elements and _empty slots_ in the __heap buffer__ at the moment of dereferencing.

Both _regular_ iterators and __safe__ iterators are contained, they will not reference memory outside the __heap buffer__, but they can dereference an _empty slot_, either that never was filled before or that did have an element that was already removed.

1. An slot that did hold and element and was removed and now empty is a __zombie__ instance. All `safememory` pointers and containers have safe zombie state.

2. An slot that never was filled before has _zeroed_ memory, so they are zeroed instances. And all `safememory` pointers and containers have safe zeroed state.


Dezombiefy 
----------
To dezombiefy iterators (that is to never dereference an _empty slot_) we must check the actual number of elements in the __heap buffer__ at the moment of dereferencing. And since that information is only know by the container instance, iterator must hold a reference to the container to ask for it.
The problem is now iterator has two references, one to the heap buffer and other to the container instance, and lifetime of them is not necesarily the same.
In particular when we look at container move-constructor and move-assignment, we realize that the lifetime of both things is independant. The instance may outlive the heap buffer (case of buffer grow) or the other way around, the heap buffer may outlive the instace (case of move-ctor).

We came to two posssible solutions to this:

* Remove move-constructor and move-assignment on container when dezombification is on. But this has issues with `owning_ptr` that is a move only type (can't copy).

* At each container instance, keep a registry where all iterators created for such instance are _subscribed_, when the container is moved (or destructed), we invalidate all existing iterators. This is the current implementation to dezombiefy iterators.

This feature has both size and computation overhead, there is a macro define `SAFEMEMORY_DEZOMBIEFY_ITERATORS` to enable it, to avoid any overhead when this feature is not required.
 
