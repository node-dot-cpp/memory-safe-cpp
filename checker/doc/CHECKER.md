
Checker tool
============


Build, run and tests
--------------------
To see how to build, run and test `nodecpp-checker`, please refer to [CHECKER-QUICK-START.md](CHECKER-QUICK-START.md), to [CHECKER-RUN.md](CHECKER-RUN.md), and to [CHECKER-TESTS.md](CHECKER-TESTS.md) for details.


Woring with templates
---------------------

Some template constructs may be tricky and many times we need to know the real instantiation parameters of a template to resolve stuff inside.
Because of that templates are not actually checked at declaration/definition, but only at instantiation.
This means that we don't check types at their `template<...> class Atype { };` but we check every variable declaration `Atype<Some> at;`, and verify that such particular instantiation complies with each required rule.
Tool has an internal _cache_ to avoid re checking already checked types.


Internal rules
--------------

Internally, some rules are implemented in terms of the place of the code being check and not in terms of the condition being evaluated. This list reflects that and tries to match internal implementation to rules declared is `STATIC-CHECKS.md`


### Rules implemented as full AST visitors

- __consistency__ Rules __C1__, __C2__ and __C3__
- __determinism__ Rules __D1__, __D2__ and __D2.1__
- __miscellaneous__ Rules __M1.1__ and __M1.2__. Types allowed at `throw` and `catch`.
- __must-co-await__ Rule __S9__, check that `[awaitable]` are actually feed to `co_await`.
- __no-side-effect__ Rule , check that functions with `[no_side_effect]` only call other functions with same attribute.
- __reference-over-co-await__ Rule __S5.8__, check that raw pointers, references and `nullable_ptr` don't survive over `co_await` or `co_yield` operators.

### Rules implemented as AST matchers

- __array-expr__ Part of __S1__, check that array subscript expressions are not used.
- __asm__ Rule __S6.1__, check that `asm` directives are not used inside code.
- __call-expr__ Rules __S1.1.1__ and __S8__, check that calls to system libraries are listed in _safe-library-db_.
- __const__ Rules __S2.1__ and __S2.2__, check that `const_cast` is not used, neither `mutable` members.
- __coroutine__ Rule __S9__, check that coroutines do return `[[awaitable]]` tagged types.
- __may-extend-lambda__ Rule __S5.7__, check lambdas passed to system calls with `[[may_extend_to_this]]` attributes.
- __naked-assignment__ Rule __S5.1__, check assignment of `nullable_ptr` and `[[naked_struct]]` to verify scope is not extended.
- __new-expr__ Rules __S4__ and __S4.1__, check that `new` and `delete` are not used, and that `make_owning` result is assigned to an `owning_ptr`. Also check that classes derived from `nodecpp::NodeBase` are not instantiated by the used.
- __raw-pointer-assignment__ Rule __S5.1__, check assignment of raw pointers don't extend scope.
- __raw-ptr-expr__ Rules __S1__, __S1.1__ and __S1.2__, only allow limited set of expressions of raw pointer type. In particular, don't allow `nullptr` to initialize a raw pointer.
- __record-decl__ Rules ..., check `class`, `struct` and `union` (non-template) declarations, to comply with _type categories_. 
- __return__ Rule __S5.1__, check that `return` statement will not return a local object.
- __static-storage__ Rule __S3__, check that __global__, `static` or `thread_local` variables are `const` and of __deep-const__ types
- __string-literal__ Rule __10.1__, check that `safe_memory::string_literal` is only constructed and assigned from string literals.
- __temporary-expr__ check temporary expressions to comply with _type categorie_, in particular important when a temporary instantiates a template holding an union inside. 
- __var-decl__ Rules __S5.3__, check that all reference variables are not initialized with `ExprWithCleanups`.
    Check that every variable and function parameter type to comply with _type categories_


Types categories
----------------

- __heap-safe__ are types deemed safe to be allocated at the heap. They are:
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



