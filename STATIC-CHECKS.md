# static checks for memory-safe-cpp

## Check Domains

In addition to checking for memory consistency, our static checker can also check for potential violations of determinism. 
Mode of check (memory safety, determinism, or both) is specified in the command line. 

## List of checks

Legend for TEST CASES:
* "i" - variable of integral type
* "p" - variable of raw pointer type (T*)
* "np" - variable of naked_ptr<T>
* "sp" - variable of soft_ptr<T>
* "op" - variable of owning_ptr<T>
* "fp()" - function taking raw pointer type (T*)
* "fop()" - function taking owning_ptr<T>
* NSTR - naked_struct type
* nstr - variable of naked_struct type

* **IMPORTANT**: whenever we're speaking of safe_ptr<T> or naked_ptr<T>, then not_null<safe_ptr<T>> and not_null<naked_ptr<T>> are ALWAYS implied (and SHOULD be included into relevant test cases)
* **IMPORTANT**: whenever we're speaking of owning_ptr<T>, safe_ptr<T> or naked_ptr<T>, then their short aliases (optr<T>, sptr<T>, and nptr<T>) are ALWAYS implied (and SHOULD be included into relevant test cases)

### Memory Safety Checks
  
* Not allowing to create pointers except in an allowed manner
  - **[Rule S1]** any (sub)expression which has a type of T* (or T&) is prohibited unless it is one of the following:
    + (sub)expression is an assignment where the right side of (sub)expression is already a pointer/reference to T (or a child class of T).
    + or (sub)expression is a dynamic_cast<> 
      * NB: MOST of C-style casts, reinterpret_casts, and static_casts (formally - all those casts between different types) MUST be prohibited under generic **[Rule S1]**, but SHOULD be reported separately under **[Rule S1.1]**
    + or (sub)expression is a function call
      * in practice, only unsafe functions can do it - but returning T* from owning_ptr<T>/soft_ptr<T>/naked_ptr<T> functions is necessary
    + or (sub)expression is nullptr
    + or (sub)expression is dereferencing of a naked_ptr<T>, soft_ptr<T>, or owning_ptr<T>
      - dereferencing of raw pointers is prohibited - and SHOULD be diagnosed as a separate **[Rule S1.2]**
    + NB: taking a variable address ("&i") is not necessary (it is done via constructor of naked_ptr<>)
    + TEST CASES/PROHIBIT: `(int*)p`, `p^p2`, `p+i`, `p[i]` (syntactic sugar for *(p+a) which is prohibited), `p1=p2=p+i`, `*nullptr`, `*p` (necessary to ensure nullptr safety)
    + TEST CASES/ALLOW: `dynamic_cast<X*>(p)`, `p=p2`, `p=np`, `p=sp`, `p=op`, `fp(p)`, `fp(np)`, `fp(sp)`, `fp(op)`, `&i`, `*np`, `*sp`, `*op`
  - **[Rule S1.1]** C-style casts, reinterpret_casts, and static_casts are prohibited. See NB in [Rule S1]. NB: this rule (which effectively prohibits even casts from X* to X*) is NOT necessary to ensure safety, but significantly simplifies explaining and diagnostics.
    + TEST CASES/PROHIBIT: `(int*)p`, `static_cast<int*>(p)`, `reinterpret_cast<int*>(p)`   
  - **[Rule S1.2]** Separate diagnostics for dereferencing of raw pointers (see above)
    + TEST CASES/PROHIBIT: `*nullptr`, `*p`
  - **[Rule S1.3]** raw pointer variables (of type T*) are prohibited; raw pointer function parameters are also prohibited. Developers should use naked_ptr<> instead. NB: this rule is NOT necessary to ensure safety, but [Rule S1] makes such variables perfectly useless (both calculating new values and dereferencing are prohibited on raw pointers) so it is better to prohibit them outright
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
  - **[Rule S4.1]** result of make_owning<>() call MUST be assigned to an owning_ptr<T> (or passed to a function taking owning_ptr<T>) 
    + TEST CASES/PROHIBIT: `make_owning<X>();`, `soft_ptr<X> = make_owning<X>();`
    + TEST CASES/ALLOW: `auto x = make_owning<X>();`, `owning_ptr<X> x = make_owning<X>();`, `fop(make_owning<X>());`
* **[Rule S5]** scope of raw pointer (T*) cannot expand
  + **[Rule S5.1]** each naked_ptr<> and each reference (T&) is assigned a scope. If there is an assignment of an object of 'smaller' scope to an object of 'smaller' one, it is a violation of this rule. Returning of pointer to a local variable is also a violation of this rule.
    + for pointers/references originating from owning_ptr<> or safe_ptr<>, scope is always "infinity"
    + for pointers/references originating from on-stack objects, scopes are nested according to lifetimes of respective objects
    + scopes cannot overlap, so operation "scope is larger than another scope" is clearly defined
    + TEST CASES/PROHIBIT: return pointer to local variable, TODO
  + **[Rule S5.2]** If we cannot find a scope of pointer/reference returned by a function, looking only at its signature - it is an error.
    + if any function takes ONLY ONE pointer (this includes safe_ptr<> and owning_ptr<>, AND `this` pointer if applicable), and returns more or one pointers/references, we SHOULD deduce that all returned pointers are of the same scope as the pointer passed to it
      - similar logic applies if the function takes ONLY ONE non-const pointer AND returns non-const pointer
      - NB: this stands because of prohibition on non-const globals
      - NB: it allows getters returning references
      - in the future, we MAY add mark-up to say which of the input pointers the returned one refers to. 
      - in the future, we MAY add type-based analysis here
    + TEST CASES/PROHIBIT: `X* x = ff(x1,x2);` where x1 and x2 have different scope
    + TEST CASES/ALLOW: `X* x = ff(x1);`, `X* x = ff(x1,x2);` where x1 and x2 have the same scope
  + **[Rule S5.3]** double raw/naked_ptrs where the outer pointer/ref is non-const, are prohibited, BOTH declared AND appearing implicitly within expressions. This also includes reference to a pointer (or to a naked_ptr<>).
    - NB: it also applies to multi-level pointers: to be valid, ALL outer pointers/references except for last one, MUST be const
    - NB: passing naked_ptrs by value is ok. Even if several naked_ptrs are passed to a function, there is still no way to mess their scopes up as long as there are no double pointers (there is no way to assign pointer to something with a larger scope).
    - NB: const reference to a pointer (and const pointer to pointer) is ok because of [Rule S2]
    - TEST CASES/PROHIBIT: `int** x;`, `&p`, `int *& x = p;`, `void ff(naked_ptr<int>& x)`
    - TEST CASES/ALLOW: `void ff(naked_ptr<int> np);`, `void ff(const_naked_ptr<int>& np);`, `const int *& x = p;`
  + **[Rule S5.4]** by default, no struct/class may contain a naked_ptr<> (neither struct/class can contain a naked_struct, neither even a safe/owning pointer to a naked_struct)
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
  + **[Rule S5.6]** There is special parameter mark-up `[[nodecpp::may_extend_to_this]]`. It applies to parameters which are naked pointers or naked structs, AND means that the scope of marked-up parameter MAY be extended to 'this' of current function. If such a parameter is specified, then the scope of `this` MUST be not-larger-than the scope of ALL the naked_ptrs within marked-up parameter
      + In the future, we MAY introduce other similar mark-up (`[[nodecpp:may_extend_to_a]]`?)
  + **[Rule S5.7]** Lambda is considered as an implicit naked_struct, containing all the naked_ptrs which are captured by reference
      - TEST CASES/PROHIBIT: `this->on()` (which is marked as `[[nodecpp:may_extend_to_this]]`) passing lambda with local vars passed by reference
      - TEST CASES/ALLOW: `this->on()` passing lambda with `this->members` captured by reference, `sort()` passing lamda with local vars captured by reference
* **[Rule S6]** Prohibit inherently unsafe things
  + **[Rule S6.1]** prohinbit asm. NB: most likely, MSVC-style ASM won't be parsed by clang-tools to start with (TODO: CHECK!), so it is only GCC/Clang asm which has to be detected and thrown away
* **[Rule S7]** Prohibit unsupported-yet things
  + **[Rule S7.1]** prohibit function pointers (in the future, will be supported via something like naked_func_ptr<> checking for nullptr)
  
### Determinism Checks

* Not allowing to convert pointers into non-pointers
  - **[Rule D1]** any (sub)expression which takes an argument of raw pointer type X* AND returns non-pointer type is prohibited, unless it is one of the following:
    + function call taking X* as parameter
    + TEST CASES/PROHIBIT: `(int)p`, `static_cast<int>(p)`
    + TEST CASES/ALLOW: `fp(p)`
    
