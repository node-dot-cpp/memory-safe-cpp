

The Dezombiefier (a.k.a. nodecpp-instrument)
============================================


The `nodecpp-instrument` tool does 5 steps to instrument the client code.

1. Include expansion.
	First all user `#include` are expanded into a copy of the `.cpp` file. We need to add macro expansion also, but is still pending.

2. Tricky expression detection and fix.
	Then we search all of the user code to find and fix *tricky expressions* (more details below).

3. Find Lvalues needing `dezombiefy` intrumentation calls.
4. Simple flow analisys to remove superfluos `dezombiefy` calls when posible.
5. Actual insertion of `dezombiefy` calls to the (already modified at step 2) code.


No false positive mode
----------------------
After our talk today, I will be adding a no-false-positive mode to the tool, and make it default.
Is very simple to do, I just was more trying to go 100% zombie free warranty.



Templates
---------

Steps 2 and 5 do the real in-place code modifications, and they both need to be careful about templates instantiations.
If the user writes a template and then instantiates it with values and with references, only one of them can be dezombiefied and the by-value implementation will result in compile error.

```cpp
template<class T>
auto doNothing(T t) { return t; }

//by-ref will try to make 'return dezombiefy(t);' instrumentation inside 'doNothing'
int i = doNothing<const int&>(42);

//by-value instantiation, will result in compiler error because of prevoius 'dezombiefy'
int j = doNothing(42);

```


To avoid such issues we analyse all instantiations of a template and verify that they all get the same set of code changes.
Otherwise the change is not applied.

However the previous algorithm is fooled by template parameter packs (`template<class ARGS...>`) and by `constexpr if()` in a way that I didn't anticipate. I added a quick workaround for them, but a better solution will be needed at some point.


Border crossing
---------------

We make a distinction between instrumented and non-instrumented code.
And while we usually think user code as instrumented and things like std C library as non-instrumented, all **our** libraries are in a grey area and we can put them in one side or the other depending on the needs.

In general terms, in instrumented code, the `this` pointer, and function arguments are zombie checked in the function body.
But, when calling non-instrumented code we need to check all of them *before* entering the call.

Making sure that function arguments (or the future `this`) are not zombie *before* entering the call is noticeable harder.
I call the extra checks we need when going from instrumented code into non-instrumented code as **border crossing**




Tricky expressions
------------------
We have 3 categories of *tricky* expressions (I call them `Z1`, `Z2` and `Z9` issues), and 2 strategies we can apply to refactor and fix them (`Op2Call` and `Unwrap`)


* `Z1` happends because of unsequenced code (at operators), both strategies can fix a `Z1` issue.
* `Z2` happends because of function arguments (and the `this` pointer) are evaluated one by one and kind of stored temporarily until all of them are evaluated and the call can takes place. Evaluating one of them can zombie other previously evaluated. Only strategy `Unwrap` can fix `Z2` issues. Is a problem only when *crossing the border*
* `Z9` happends because of copy/move constructors when an object is passed by-value is always executed last. If the by-value object is user defined, non trivially copyable, and we are *crossing the border* (some weird templates may cause it) then we get a `Z9` issue. No strategy can fix it.


```cpp
struct SafeType {

    void call(int) {}
};

int safeFunc(SafeType&);

struct Bad {

    owning_ptr<SafeType> s;

    Bad() { s = make_owning<SafeType>(); }

    int release() {
        s = nullptr;
        return 0;
    }

    void problemZ1() {

        safeFunc(*s) + release();
    }

    void problemZ2() {

        s->call(release());
    }
}; 
```


The strategies
--------------

* `Op2Call` replaces arithmentic and logic operators by function calls (op `+` by a function call `add()` and so), efectivelly removing `Z1` issues. This strategy can be applied to any expression in the source code.
* `Unwrap` better seen by example below, can be applied only in a few places, like stand alone stmt expressions, intializer of a variable declaration, or the expression inside an `if` stmt. Other places may be added later, but not easy (expression inside loop condition, `else if` condition, constructor initalizer expressions will be very dificult to implement). Removes `Z1` and `Z2` issues.

```cpp
	int i = getSt().call(release());

	// gets transformed into the following (shown code before dezombiefy is applied):
	auto& nodecpp_0 = getSt(); auto nodecpp_1 = release(); int i = nodecpp_0.call(nodecpp_1);

```


Coverage
--------
The only situation we can't do 100% zombie check are:
 * when *crossing the border* and we find a `Z9`
 * when *crossing the border* and we find a `Z2` in a place we can't use the `Unwrap` strategy.

So the exposed area we can't cover is really small, and since the **border** is not really fixed but we can play puting parts of our library on one side or the other, and also most (if not all) our libraries won't take arguments that may zombie (no raw ptrs or refs but still we need to worry about zombie `this` pointer), then I believe the overall coverage will be excelent.


TODO list
---------
Only the first item is really a need.

* finish *no-false-positive* mode (one day max)
* macro expansion
* solution for template param pack, and constexpr if
* better mechanism to define the **border**






