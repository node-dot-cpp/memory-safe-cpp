
Instrumentation tool
============================


Build, run and tests
--------------------
The `safememory-instrument` tool is built, run and tested in same way as `safememory-checker`. Please refer to [CHECKER-QUICK-START.md](CHECKER-QUICK-START.md), to [CHECKER-RUN.md](CHECKER-RUN.md), and to [CHECKER-TESTS.md](CHECKER-TESTS.md) for details.


Silent mode
-----------
The `safememory-instrument` tool runs by default in __silent mode__, this is a best effort mode, where the tool tries to instrument as much as it can, but it will silently ignore any expression or statemnt that is too complex to analyze and/or instrument.

We can revert this behaviour with a command line option (`-no-silent-mode`) and the tool will issue error messages at each place it did give up in trying to instrument. 



Working internals
-----------------


The `safememory-instrument` tool does 3 steps to instrument the client code.

1. Include expansion: First all user `#include` are expanded into a copy of the `.cpp` file.

2. Z-issues detection and fix: Then we detect expression with Z-issues (more below) and refactor them as apropiate.

3. Insert `dezombiefy` calls: Last we detect l-values needing dezombiefication, do a simple flow analysis to avoid superfluos calls when possible, and insert the actual calls to the (already modified at previous stage) code.


As a result, we get a _dezombiefied_ `.cpp` file, where user (and not system) `#include` has already been expanded. Such file must then be compiled with the target C++ compiler.


Woring with templates
---------------------

Steps 2 and 3 do the real in-place code modifications, and they both need to be careful about templates instantiations.
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

However the previous algorithm is fooled by template parameter packs (`template<class ARGS...>`) and by `constexpr if()`, and we can't instrument them right now.


Border crossing
---------------

We make a distinction between instrumented and non-instrumented code.
And while we usually think user code as instrumented and things like std C library as non-instrumented, all __nodecpp__ libraries are in a grey area and we can put them in one side or the other depending on the needs.

In general terms, in instrumented code, the `this` pointer, and function arguments are zombie checked inside the function body itself, before actually accessing them.
But, when calling non-instrumented code we need to check all of them _before_ entering the call.

Making sure that function arguments (or the future `this`) are not zombie _before_ entering the call is noticeable harder to do.
I call those extra checks we need to insert when jumping from instrumented code into non-instrumented code as __border crossing__




Z-issues
--------
We have 3 categories of _Z-issues_ we can find in expressions (I call them `Z1`, `Z2` and `Z9`), and 2 strategies we can apply to refactor and fix them (`Op2Call` and `Unwrap`).


* `Z1` happends at unsequenced code at evaluation of operators arguments. Both strategies can fix a `Z1` issue.
* `Z2` happends at sequenced (or indeterminatelly sequenced) evaluation of function arguments (and also the callee). Evaluations are done one by one and their result is stored. And because of that one evaluation can make _zombie_ the result of a previous one. Only strategy `Unwrap` can fix `Z2` issues. Is a problem only when _crossing the border_
* `Z9` happends because of copy/move constructors when an object is passed by-value is always executed last. If the by-value object is user defined, non trivially copyable, and we are _crossing the border_ (some templates may do it) then we get a `Z9` issue. No strategy can fix it.



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

        // 'release()' and '*s' are unsequenced
        safeFunc(*s) + release();
    }

    void problemZ2() {

        // 's->' is sequenced before 'release()'
        s->call(release());
    }
}; 
```

The strategies
--------------

* `Op2Call` replaces arithmentic and logic operators by function calls (op `+` by a function call `add()` and so), efectivelly removing `Z1` issues. This strategy can be applied to any expression in the source code.
* `Unwrap` better seen by example below, can be applied only in a few places, like stand alone stmt expressions, intializer of a variable declaration, or the expression inside an `if` stmt. Other places may be added later, but not easy (expression inside loop condition, `else if` condition, constructor initalizer expressions will be very dificult to implement). Removes any `Z1` and `Z2` issues existing in the expression.

```cpp
	int i = getSt().call(release());

	// gets transformed into the following (shown code before dezombiefy is applied):
	auto& nodecpp_0 = getSt(); auto nodecpp_1 = release(); int i = nodecpp_0.call(nodecpp_1);

```


Coverage
--------
Th situation we can't do zombie check are:
 * when *crossing the border* and we find a `Z9`
 * when *crossing the border* and we find a `Z2` in a place we can't use the `Unwrap` strategy.

