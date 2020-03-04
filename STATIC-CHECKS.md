# static checks for memory-safe-cpp

## Scope, #includes, and Check Domains

### Check Domains
In addition to checking for memory consistency, our static checker can also check for potential violations of determinism. 
Mode of check (memory safety, determinism, or both) is specified in the command line. TODO: non-memory UB checks, cross-platform checks.

### Scope of analysis
* all files included via `#include <>` are considered library files. Out of functions declared in library files, ONLY functions which protoypes are explicitly listed in a special file safe_library.h, are allowed. safe_library.h MUST include ALL safe functions from C library. TODO: safe classes (with ALL safe functions of the class also explicitly listed)
* all files included via `#include ""` are considered project files. For such "project files":
    - by default, all the classes/functions are considered 'safe', and are analysed for safety
    - if a class/function is labeled as [[nodecpp::memory_unsafe]] , *or belongs to a namespace labeled as [[nodecpp::memory_unsafe]]*, memory safety checks are skipped for this class/function
    - if a class/function is labeled as [[nodecpp::non_deterministic]] , *or belongs to a namespace labeled as [[nodecpp::non_deterministic]]*, determinism checks are skipped for this class/function
    - NB: first, have to double-check that attributes on namespaces are really allowed by all three major compilers

## List of checks

Legend for TEST CASES:
* "i" - variable of integral type
* "p" - variable of raw pointer type (T*)
* "r" - variable of reference type (T&)
* "np" - variable of `naked_ptr<T>` type
* "sp" - variable of `soft_ptr<T>` type
* "op" - variable of `owning_ptr<T>` type
* "fp()" - function taking raw pointer type (T*)
* "fop()" - function taking `owning_ptr<T>`
* NSTR - naked_struct type
* nstr - variable of naked_struct type
* af(), af2() - asynchronous function returning nodecpp::awaitable<>

* **IMPORTANT**: whenever we're speaking of `safe_ptr<T>` or `naked_ptr<T>`, then `not_null<safe_ptr<T>>` and `not_null<naked_ptr<T>>` are ALWAYS implied (and SHOULD be included into relevant test cases)
* **IMPORTANT**: whenever we're speaking of `owning_ptr<T>`, `safe_ptr<T>` or `naked_ptr<T>`, then their short aliases (`optr<T>`, `sptr<T>`, and `nptr<T>`) are ALWAYS implied (and SHOULD be included into relevant test cases)

### Consistency Checks

Consistency checks always apply (regardless of the command line, and any attributes)

* **[Rule C1]** ONLY those [[nodecpp::]] attributes which are specified by this document, are allowed. Using of unspecified [[nodecpp::]] attribute is an error.
  - TEST CASES/PROHIBIT: `[[nodecpp::abracadabra]]`
* **[Rule C2]** use of [[nodecpp::]] attributes is allowed ONLY in those places specified by this document. Using of [[nodecpp::]] attributes in a wrong place is an error. 
  - TEST CASES/PROHIBIT: `[[nodecpp::naked_struct]] void f();`
* **[Rule C3]** if some namespace has [[nodecpp::memory_unsafe]] or [[nodecpp::non_deterministic]] attribute, these attributes MUST be the same for ALL the instances of the same namespace. 
  - TEST CASES/PROHINIT: `namespace [[nodecpp:memory_unsafe]] abc {}; namespace abc {};`

### Memory Safety Checks
  
* Not allowing to create pointers except in an allowed manner
  - **[Rule S1]** any (sub)expression which has a type of T* (or T&) is prohibited unless it is one of the following:
    + (sub)expression is an assignment where the right side of (sub)expression is already a pointer/reference to T (or a child class of T).
    + or (sub)expression is a `dynamic_cast<>` 
      * NB: MOST of C-style casts, reinterpret_casts, and static_casts (formally - all such casts between different types) MUST be prohibited under generic **[Rule S1]**, but SHOULD be reported separately under **[Rule S1.1]**
    + or (sub)expression is a function call
      * in practice, only unsafe functions can do it - but returning T* from `owning_ptr<T>`/`soft_ptr<T>`/`naked_ptr<T>` functions is necessary
    + or (sub)expression is _nullptr_
    + or (sub)expression is _this_
    + or (sub)expression is dereferencing of a `naked_ptr<T>`, `soft_ptr<T>`, or `owning_ptr<T>`
      - dereferencing of raw pointers is prohibited - and SHOULD be diagnosed as a separate **[Rule S1.2]**
    + NB: taking a variable address ("&i") is not necessary (it is done via constructor of `naked_ptr<>`)
    + TEST CASES/PROHIBIT: `(int*)p`, `p^p2`, `p+i`, `p[i]` (syntactic sugar for *(p+a) which is prohibited), `p1=p2=p+i`, `*nullptr`, `*p` (necessary to ensure nullptr safety)
    + TEST CASES/ALLOW: `dynamic_cast<X*>(p)`, `p=p2`, `p=np`, `p=sp`, `p=op`, `fp(p)`, `fp(np)`, `fp(sp)`, `fp(op)`, `&i`, `*np`, `*sp`, `*op`
  - **[Rule S1.1]** C-style casts, reinterpret_casts, and static_casts are prohibited. See NB in [Rule S1]. NB: if [Rule S1] is already enforced, this rule [Rule S1.1] (which effectively prohibits even casts from X* to X*) is NOT necessary to ensure safety, but significantly simplifies explaining and diagnostics.
    + **[Rule S1.1.1]** special casts, in particular `soft_ptr_static_cast<>`. are also prohibited in safe code 
    + TEST CASES/PROHIBIT: `(int*)p`, `static_cast<int*>(p)`, `reinterpret_cast<int*>(p)`, `soft_ptr_static_cast<X*>(p)`   
  - **[Rule S1.2]** Separate diagnostics for dereferencing of raw pointers (see above)
    + TEST CASES/PROHIBIT: `*nullptr`, `*p`
  - **[Rule S1.3]** raw pointer variables (of type T*) are prohibited; raw pointer function parameters are also prohibited. Developers should use `naked_ptr<>` instead. NB: this rule is NOT necessary to ensure safety, but [Rule S1] makes such variables perfectly useless (both calculating new values and dereferencing are prohibited on raw pointers) so it is better to prohibit them outright
    + NB: raw references are ok (we're ensuring that they're not null in the first place)
    + TEST CASES/PROHIBIT: `int* x;`
  - **[Rule S1.4]**. Unions with any raw/naked/soft/owning pointers (including any classes containing any such pointers) are prohibited
    + TEST CASES/PROHIBIT: `union { naked_ptr<X> x; int y; }`
    + TEST CASES/ALLOW: `union { int x; long y; }`
* **[Rule S2]** const-ness is enforced
  + **[Rule S2.1]** const_cast is prohibited
  + **[Rule S2.2]** mutable members are prohibited
  + TEST CASE/PROHIBIT: `const_cast<X*>`, `mutable int x;`
* **[Rule S3]** non-constant global variables, static variables, and thread_local variables are prohibited. NB: while prohibiting thread_local is not 100% required to ensure safety, it is still prohibited at least for now.
  + const statics/globals are ok (because of [Rule S2])
  + NB: this rule effectively ensures that all our functions are no-side-effect ones (at least unless explicitly marked so)
  + TEST CASES/PROHIBIT: `int x;` at global scope, `thread_local int x;`, `static int x;` within function, `static int x;` within the class
  + TEST CASES/ALLOW: `static void f();` within class, free-standing `static void f();`, `static constexpr int x;`, `static const int x;` (both in class and globally)
* **[Rule S4]** new operator (including placement new) is prohibited (developers should use make_owning<> instead); delete operator is also prohibited
  + TEST CASES/PROHIBIT: `new X()`, `new int`
  + TEST CASES/ALLOW: `make_owning<X>()`, `make_owning<int>()`
  - **[Rule S4.1]** result of `make_owning<>()` call MUST be assigned to an `owning_ptr<T>` (or passed to a function taking `owning_ptr<T>`) 
    + TEST CASES/PROHIBIT: `make_owning<X>();`, `soft_ptr<X> = make_owning<X>();`
    + TEST CASES/ALLOW: `auto x = make_owning<X>();`, `owning_ptr<X> x = make_owning<X>();`, `fop(make_owning<X>());`
* **[Rule S5]** scope of raw pointer (T*) cannot expand [[**TODO/v0.5: CHANGE Rule S5 completely to rely on Herb Sutter's D1179: https://github.com/isocpp/CppCoreGuidelines/blob/master/docs/Lifetime.pdf; NB: SOME of the rules below may still be needed on top of D1179 **]]
  + **[Rule S5.1]** each `naked_ptr<>`, reference (T&) or naked_struct is assigned a scope. If there is an assignment of an object of 'smaller' scope to an object of 'smaller' one, it is a violation of this rule. Returning of pointer to a local variable is also a violation of this rule.
    + for pointers/references originating from `owning_ptr<>` or `safe_ptr<>`, scope is always "infinity"
    + for pointers/references originating from on-stack objects, scopes are nested according to lifetimes of respective objects
    + scopes cannot overlap, so operation "scope is larger than another scope" is clearly defined
    + TEST CASES/PROHIBIT: return pointer to local variable, TODO
  + **[Rule S5.2]** If we cannot find a scope of pointer/reference returned by a function, looking only at its signature - it is an error.
    + if any function takes ONLY ONE pointer (this includes `safe_ptr<>` and `owning_ptr<>`, AND `this` pointer if applicable), and returns more or one pointers/references, we SHOULD deduce that all returned pointers are of the same scope as the pointer passed to it
      - similar logic applies if the function takes ONLY ONE non-const pointer AND returns non-const pointer
      - NB: this stands because of prohibition on non-const globals
      - NB: it allows getters returning references
      - in the future, we MAY add mark-up to say which of the input pointers the returned one refers to. 
      - in the future, we MAY add type-based analysis here
    + TEST CASES/PROHIBIT: `X* x = ff(x1,x2);` where x1 and x2 have different scope
    + TEST CASES/ALLOW: `X* x = ff(x1);`, `X* x = ff(x1,x2);` where x1 and x2 have the same scope
  + **[Rule S5.3]** double raw/naked_ptrs where the outer pointer/ref is non-const, are prohibited, BOTH declared AND appearing implicitly within expressions. This also includes reference to a pointer (or to a `naked_ptr<>`).
    - NB: it also applies to multi-level pointers: to be valid, ALL outer pointers/references except for last one, MUST be const
    - NB: passing naked_ptrs by value is ok. Even if several naked_ptrs are passed to a function, there is still no way to mess their scopes up as long as there are no double pointers (there is no way to assign pointer to something with a larger scope).
    - NB: const reference to a pointer (and const pointer to pointer) is ok because of [Rule S2]
    - TEST CASES/PROHIBIT: `int** x;`, `&p`, `int *& x = p;`, `void ff(naked_ptr<int>& x)`
    - TEST CASES/ALLOW: `void ff(naked_ptr<int> np);`, `void ff(const_naked_ptr<int>& np);`, `const int *& x = p;`
  + **[Rule S5.4]** by default, no struct/class may contain naked_ptrs, raw pointers or references  (neither struct/class can contain a naked_struct, neither even a safe/owning pointer to a naked_struct)
    - if a struct/class is marked up as `[[nodespp::naked_struct]]`, it may contain naked_ptrs (but not raw pointers), and other naked_structs by value; it still MUST NOT contain raw/naked/safe/owning pointers to a naked_struct
    - allocating naked_struct on heap is prohibited
    - NB: having raw pointers (T*) is prohibited by [Rule S1.3]
    - TEST CASES/PROHIBIT: `struct X { naked_ptr<Y> y; };`, `[[nodecpp:naked_struct]] struct X { soft_ptr<NSTR> y; };`, `make_owning<NSTR>()`
    - TEST CASES/ALLOW: `struct X { soft_ptr<Y> y; };`, `[[nodecpp:naked_struct]] struct X { naked_ptr<Y> y; };`
  + **[Rule S5.5]** Creating a non-const pointer/reference to a naked_struct (or passing naked_struct by reference) is prohibited (it would violate [Rule S5.1]).
    - This implies prohibition on member functions of naked_struct (as `this` parameter is always a pointer), except for `const` ones
    - NB: passing naked_struct by value is ok - and should be treated as passing several naked_ptrs (with their respective scopes)
    - TEST CASES/PROHIBIT: `naked_ptr<NSTR>`, `void ff(NSTR&)`
    - TEST CASES/ALLOW: `const_naked_ptr<NSTR>`, `void ff(const NSTR&)`
  + **[Rule S5.6]** Lambda is considered as an implicit _naked_struct_, captures may include local var references, _naked_ptrs_ and raw pointer `this`.
    - TEST CASES/ALLOW: `sort()` passing lamda with local vars captured by reference
  + **[Rule S5.7]** There is special parameter mark-up `[[nodecpp::may_extend_to_this]]` that may be applied to method parameter of type `std::function` or _lambda_ on library code api (or `[[nodecpp::memory_unsafe]]` marked code), AND means that the scope of marked-up parameter MAY be extended to `this` of called instance. If such a parameter is specified, then the scope of the captures MUST be equal-or-larger-than the scope of the called instance.
    - When applied to parameter of type `std::function`, it means it can only be initialized with a lambda that verifies the mentioned retrictions.
    - In the future, we MAY introduce other similar mark-up (`[[nodecpp::may_extend_to_a]]`?)
    - TEST CASES/PROHIBIT: `this->on()` (which is marked as `[[nodecpp::may_extend_to_this]]`) passing lambda with local vars passed by reference
    - TEST CASES/ALLOW: `this->on()` passing lambda with `this->members` captured by reference
  + **[Rule S5.8]** naked_ptr<>s and references MUST NOT survive over co_await
      - TEST CASES/PROHIBIT: `co_await some_function(); auto x = *np;`, `co_await some_function(); auto x = r;`
      - TEST CASES/ALLOW: `co_await some_function(); auto x = *sp;`  
* **[Rule S6]** Prohibit inherently unsafe things
  + **[Rule S6.1]** prohibit asm, both MSVC and GCC style.
* **[Rule S7]** Prohibit unsupported-yet things
  + **[Rule S7.1]** prohibit function pointers (in the future, will be supported via something like naked_func_ptr<> checking for nullptr)
* **[Rule S8]** Prohibit functions except for those safe ones. 
  + Out of functions declared in "library files" (those included via `#include <>`), ONLY functions which protoypes are explicitly listed in a special safe-library header file (specified in a command line to the compiler such as safe-library=safe_library.h), are allowed. safe-library header MUST include ALL safe functions from C library.
    - safe-library header should allow specifying safe classes (with ALL safe member functions within the class also explicitly listed; whatever-functions-are-unlisted, are deemed unsafe)
    - safe-library header should support `#include` within; it should support BOTH (a) including other safe-library headers, and (b) including real library files (such as our own soft_ptr.h). TBD: for (b), there should be a way to restrict `#include` within safe-library header to only direct #include (without nested #includes).
    - template types and functions listed as safe are so independant of its parameters. Control of allowed template parameters must be done at a different level (i.e. using enable_if<> or concepts).
    - functions and methods listed as safe are so independant of its arguments or overloads.
    - TODO: support for `strlen()` etc.
    - TEST CASES/PROHIBIT: `memset(p,1,1)`
    - TEST CASES/ALLOW: `soft_ptr<X*> px;`
  + All the functions from "project files" (those included via `#include ""`) are ok (even if they're labeled with [[nodecpp::memory_unsafe]]). It is a responsibility of the developers/architects to ensure that [[nodecpp::memory_unsafe]] functions are actually safe.
* **[Rule S9]** Miscellaneous checks
  + **[Rule S9.1]** nodecpp::awaitable<>/co_await consistency (necessary to prevent leaks). For any function f, ALL return values of ALL functions/coroutines returning nodecpp::awaitable<>, MUST be fed to co_await operator within the same function f, and without any conversions. In addition, such return values MUST NOT be copied, nor passsed to other functions (except for special function wait_for_all())
    - TEST CASES/PROHIBIT: `af();`, `{ auto x = af(); }`, `int x = af();`, `auto x = af(); another_f(x); /* where another_f() takes nodecpp::awaitable<> */`, `auto x = af(); auto y = x;` 
    - TEST CASES/ALLOW: `co_await af();`, `int x = co_await af();`, `auto x = af(); auto y = af2(); co_await x; co_await y;`, `nodecpp::awaitable<int> x = af(); co_await x;`, `co_await wait_for_all(af(), af2())`
* **[Rule S10]** Prohibit using unsafe collections and iterators
  + Collections, such as `std::vector<...>`, `std::string`, etc,  and iterators internally use unsafe memory management, and, therefore, nmust be prohibited. Safe collections (such as `nodecpp::vector<...>` should be used instead.
    - TEST CASES/PROHIBIT: `std::vector<...> v`, `std::string s`, etc; ` 
    - TEST CASES/ALLOW: `nodecpp::vector<...> v`, `nodecpp::string s`, etc; 
  + **[Rule S10.1]** support StringLiteral class - it can be created ONLY from string literal, OR from another string literal.
    - TEST CASES/PROHIBIT: `const char* s = "abc"; StringLiteral x = s;`, `void fsl(StringLiteral x) {} ... const char* s = "abc"; fsl(s);`
    - TEST CASES/ALLOW: `StringLiteral x = "abc";`, `void fsl(StringLiteral x) {} ... fsl("abc");`, `void fsl(StringLiteral x) {} ... StringLiteral x = "abc"; fsl(x);`

### Determinism Checks (strictly - ensuring Same-Executable Determinism)

* Not allowing to convert pointers into non-pointers
  - **[Rule D1]** any (sub)expression which takes an argument of raw pointer type X* AND returns non-pointer type is prohibited, unless it is one of the following:
    + function call taking X* as parameter
    + convertion to `bool`
    + TEST CASES/PROHIBIT: `(int)p`, `static_cast<int>(p)`
    + TEST CASES/ALLOW: `fp(p)`, `if(!p)`
* **[Rule D2]** Prohibiting uinitialized variables, including partially unitialized arrays
  - TEST CASES/PROHIBIT: `int x;` (in function), `int a[3] = {1,2};` (in function), `X x;` (in function, if class X has non-constructed members AND has no constructor), `int x1 : 8;` (in function, this is an uninitialized bit field)
  - TEST CASES/ALLOW: `int x = 0;` (in function), `X x;` (in function, provided that class X has default constructor), `int a[3] = {1,2,3};` (in function), `int x1 : 8 = 42;` (in function, this is an initialized bit field)
  - **[Rule D2.1]** Prohibiting uninitialized class members; at the moment, ONLY C++11 initializers next to data member are recognized. TODO: allow scenarios when ALL the constructors initialize data member in question. 
    + TEST CASES/PROHIBIT: `int x;` (as data member), `int a[3] = {1,2};` (as data member), `X x;` (as data member, if class X has has non-constructed members AND has no constructor), `int x1 : 8;` (as data member, this is an uninitialized bit field)
    + TEST CASES/ALLOW: `int x = 0;` (as data member), `X x;` (as data member, provided that class X has default constructor), `int a[3] = {1,2,3};` (as data member), `int x1 : 8 = 42;` (as data member, this is an initialized bit field)

### Miscellaneios Checks (in particular, coding style we want to encourage)
* **[Rule M1]** Only nodecpp::error can be thrown/caught (NO derivatives)
  - **[Rule M1.1]** Only nodecpp::error can be thrown
      - TEST CASES/ALLOW: `throw nodecpp::error(nodecpp::errc::bad_alloc);`, `throw nodecpp::error::bad_alloc;`
      - TEST CASES/PROHIBIT: `throw 0;`, `throw std::exception`, `throw std::error;`
  - **[Rule M1.2]** Only nodecpp::exception can be caught (and ONLY by reference)
      - TEST CASES/ALLOW: `catch(nodecpp::error& x)`
      - TEST CASES/PROHIBIT: `catch(int)`, `catch(std::exception)`, `catch(std::error)`, `catch(nodecpp::error)`
* **[Rule M2]** Ensuring code consistency regardless of tracing/assertion levels
  - **[Rule M2.1]** Within NODETRACE* macros, there can be ONLY const expressions
      - TEST CASES/ALLOW: `NODETRACE3("{}",a==b);`
      - TEST CASES/PROHIBIT: `NODETRACE3("{}",a=b);`
  - **[Rule M2.2]** Within NODEASSERT* macros, there can be ONLY const expressions
      - TEST CASES/ALLOW: `NODEASSERT2(a==b);`
      - TEST CASES/PROHIBIT: `NODEASSERT2(a=b);`
