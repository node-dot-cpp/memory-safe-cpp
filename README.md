# memory-safe-cpp
A dialect of C++ with memory safety guarantees

## Principles 

Principles behind are discussed in http://ithare.com/a-usable-c-dialect-that-is-safe-against-memory-corruption/ , 
though some implementation details here are different; in particular:
* "soft pointers" are implemented via double-linked-lists from owning pointers, with non-trivial move constructors for both owning pointers and soft pointers 
  * an optimization for stack-only pointers pending 

## Goals

Eventually we're planning to make this a memory-safe C++, in a sense that:
* IF you're following certain rules
  * To help with it, we're planning to include our own static analyzer, enforcing the rules outlined in the article 
* AND compile your program with certain settings (such as #defining NODECPP_MEMORY_SAFETY)
* THEN we guarantee that your C++ program does not exhibit any memory-related Undefined Behaviors
  * of course, saving for bugs in our tools, but with time we hope to make it very solid

Safety checks are two-fold:
* compile-time checks.
* run-time checks. run-time checks do incur performance penalty
  * due to the model chosen, they're rare, and for most of the programs we expect them not to be TOO bad
  * in addition, there is an option to re-compile your program without safety checks. Or even with per-pointer/per-container safety checks.

## Current Status

We have just started, need some time to produce anything which is worth a look :-( . 
