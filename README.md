# memory-safe-cpp
A dialect of C++ with memory safety guarantees

## Principles 

Principles behind are discussed in http://ithare.com/a-usable-c-dialect-that-is-safe-against-memory-corruption/ , 
though some implementation details here are different; in particular:
* "soft pointers" are implemented via vectors of soft pointers within owning pointers, with non-trivial move constructors for soft pointers
  * this ensures an almost-zero cost of dereferencing a soft pointer, at the cost of slowing down copying/destruction of "soft pointers" (but not by much)
  * an optimization for stack-only soft-pointers still pending 
* X* pointers are prohibited, naked_ptr<> has to be used instead (to enforce safety against nullptr regardless of relying on 'zero page' protection)

## Goals

This project aims to provide a memory-safe C++, in a sense that:
* **IF** you're following certain rules (="your code passes our static checker")
* **AND** you compile your program with certain settings (such as not #defining NODECPP_MEMORY_SAFETY=NONE)
* **THEN we guarantee that your C++ program does not exhibit any memory-related Undefined Behaviors**
  * of course, saving for bugs in our tools, but with time we hope to make it very solid

Safety checks are two-fold:
* compile-time checks. compile-time checks do NOT incur performance penalty. Used to enforce stack safety (example: return of pointer to local variable)
* run-time checks. run-time checks do incur performance penalty. Used to enforce safety of the heap. 
  * due to the model chosen, they're rare, and for most of the programs we expect them not to be TOO bad
  * in addition, there is an option to re-compile your program without safety checks. Or even with per-class/per-pointer safety checks.

## Current Status and Further Plans

* We are about to release v0.1 - which is in "no known bugs" status, and is supposed to be safe. v0.1 does NOT support stuff such as arrays or collections (at all).
  * This is **pre-alpha** version, so while all the bug reports are REALLY welcome, **please do NOT say "hey, they didn't even handle <insert_trivial_thing_here>, so they're hopeless"** - at this point we are confident that we'll be able to fix all the bugs reported to us, but it will take some time. 
* FURTHER PLANS: 
  * v0.1.x - bugfixes and more bugfixes, support for co_await
  * v0.2 Adding support for strings, arrays, spans, vectors, and hash tables
  * v0.5 Switching to detection mechanisms outlined in D1179 (splitting D1179's _invalid_ into _invalid_stack_ - reported as error, and _invalid_heap_ - reported only as warning as long as the runtime protection is enabled).
  * v0.6 Adding support for other collections (deques and tree-based ones).
  * v0.8 Adding instrumentation to enable 100% run-time detection of zombie accesses. 
  
