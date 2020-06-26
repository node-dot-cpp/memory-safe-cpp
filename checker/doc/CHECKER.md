
Checker tool
============


Build, run and tests
--------------------
To see how to build, run and test `nodecpp-checker`, please refer to [CHECKER-QUICK-START.md](CHECKER-QUICK-START.md), to [CHECKER-RUN.md](CHECKER-RUN.md), and to [CHECKER-TESTS.md](CHECKER-TESTS.md) for details.


Known issues
------------

- Rule __S5.3__, multi level pointers and references are not allowed, only one outer _const reference_ is allowed and unwrapped (treated as if _by-value_).
- Rule __S10.3.1__, allow to call `TRACE` inside __no_side_effect__, is not implemented yet
- Rule __M2__, consistency regardless of `TRACE` and `ASSERT` is not implemented yet, what to do/how to prevent exceptions thrown.



Woring with templates
---------------------

Some template constructs may be tricky and many times we need to know the real instantiation parameters of a template to resolve stuff inside.
Because of that templates are not actually checked at declaration/definition, but only at instantiation.
This means that we don't check types at their `template<...> class Atype { };` but we check every variable declaration `Atype<Some> at;`, and verify that such particular type instantiation complies with each required rule.
Tool has an internal _cache_ to avoid re checking already checked types.


Internal rules
--------------

Internally, some rules are implemented in terms of the place of the code being check and not in terms of the condition being evaluated. This is so, because we want to emit a single error message for each place in the code (the one we believe most relevant), and because of template instantiations force as to do so in some situations. This list reflects that and tries to match internal implementation to rules declared is `STATIC-CHECKS.md`


### Rules implemented as full AST visitors

- __consistency__ Rules __C1__, __C2__ and __C3__
- __determinism__ Rules __D1__, __D2__ and __D2.1__
- __miscellaneous__ Rules __M1.1__ and __M1.2__. Types allowed at `throw` and `catch`.
- __must-co-await__ Rule __S9__, check that `[awaitable]` are actually feed to `co_await`.
- __no-side-effect__ Rule __S10.3__, check that functions with `[no_side_effect]` only call other functions with same attribute.
- __reference-over-co-await__ Rule __S5.8__, check that raw pointers, references and `nullable_ptr` don't survive over `co_await` or `co_yield` operators.

### Rules implemented as AST matchers

- __array-expr__ Part of __S1__, check that array subscript expressions are not used. While `A[i]` is syntax sugar to `*(A + i)`, the first syntax is't actually a raw pointer expression in the AST, so it must be checked independently.
- __asm__ Rule __S6.1__, check that `asm` directives are not used inside code.
- __call-expr__ Rules __S1.1.1__ and __S8__, check that calls to system libraries are listed in _safe-library-db_.
- __const__ Rules __S2.1__ and __S2.2__, check that `const_cast` is not used, neither `mutable` members.
- __coroutine__ Rule __S9__, check that coroutines do return `[[awaitable]]` tagged types.
- __may-extend-lambda__ Rule __S5.7__, check lambdas passed to system calls with `[[may_extend_to_this]]` attributes.
- __naked-assignment__ Rule __S5.1__, check assignment of `nullable_ptr` and `[[naked_struct]]` to verify scope is not extended.
- __new-expr__ Rules __S4__ and __S4.1__, check that `new` and `delete` are not used, and that `make_owning` result is assigned to an `owning_ptr`. Also check that classes derived from `nodecpp::NodeBase` are not instantiated by the used.
- __raw-pointer-assignment__ Rule __S5.1__, check assignment of raw pointers don't extend scope.
- __raw-ptr-expr__ Rules __S1__, __S1.1__, __S1.2__ and __S1.3__, only allow limited set of expressions of raw pointer type. In particular, don't allow `nullptr` to initialize a raw pointer.
- __record-decl__ Rules for __type categories__ below, check `class`, `struct` and `union` (non-template) declarations, to comply. 
- __return__ Rule __S5.2__, check that scope of `return` statement is in line with scope of input arguments (including `this` when applies).
- __static-storage__ Rule __S3__, check that __global__, `static` or `thread_local` variables are `const` and of __deep-const__ types
- __string-literal__ Rule __10.1__, check that `safe_memory::string_literal` is only constructed and assigned from string literals.
- __temporary-expr__ Rules for __type categories__ below, check temporary expressions to comply. Particular important when a temporary instantiates a template holding an union inside. 
- __var-decl__ Rules for __type categories__ below, check that every variable and function parameter type to comply.
    Also rule __S5.1__, check that reference variables are not initialized with _lifetime extended_ references.



Types categories
----------------
From the understanding of rules __S1.4__, __S5.3__, __S5.4__, __S5.5__, __S5.6__, __S8__ and __S10.2__ the tool divides types into the following categories. Some of them are checked as a single thing rather than individual rules. 


- __heap-safe__ are types deemed safe to be allocated at the heap (or the stack). They are:
    - _primitive_ types.
    - _safe library_ types (those in safe-library-db).
    - `class` or template class with all members and bases being _heap-safe_ types.
    - __owning-ptr__ of _heap-safe_ type.
    - __soft-ptr__ of _heap-safe_ type.
    - __safe-union__, `union` of _primitive_ types (Rule __S1.4__).


    By default, all `class`, `struct` and `union` types are checked to be _heap-safe_ unless some attribute tags them otherwise.



- __deep-const__ types are a subset of _heap-safe_ types, that have a deep `const` semantic. They are:
    - _primitive_ types.
    - _safe library_ types with `[[deep_const]]` attribute.
    - `class` or template class with all members and bases being _deep-const_ types, and having `[[deep-const]]` attribute
    - __owning-ptr__ of `const` _deep-const_ type.

- __stack-only__ types are those allowed only at the stack, at variables and function parameters. They have some special rules of scope and around _coroutines_. They are:

    - _raw pointer_ to _heap-safe_ types.
    - _reference_ to _heap-safe_ types.
    - __nullable-ptr__ of _heap-safe_ types.
    - __naked-struct__, `class`  or template class with all members of __nullable_ptr__ type, and having attribute `[[naked_struct]]` (Rule __S5.4__).
    - _const reference_ to _stack-only_ types.
    - _lambdas_ (may capture `this` and local references).


Other types are not allowed at all.


Scope analysis
--------------
To enforce rules __S5.1__ and __S5.2__ the tools _digs_ into full expressions to find all the involved scopes.
The _output_ scope of a complex expression is equal to the minimun of all of its _input_ scopes.
This analysis is performed when a reference or pointer variable is assigned.





