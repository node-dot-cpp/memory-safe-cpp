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

* **IMPORTANT**: whenever we're speaking of safe_ptr<T> or naked_ptr<T>, then not_null<safe_ptr<T>> and not_null<naked_ptr<T>> are ALWAYS implied (and SHOULD be included into relevant test cases)
* **IMPORTANT**: whenever we're speaking of owning_ptr<T>, safe_ptr<T> or naked_ptr<T>, then their short aliases (optr<T>, sptr<T>, and nptr<T>) are ALWAYS implied (and SHOULD be included into relevant test cases)

### Memory Safety Checks
  
* Not allowing to create pointers except in an allowed manner
  - **[Rule S1]** any (sub)expression which has a type of T* (or T&) is prohibited unless it is one of the following:
    + (sub)expression is an assignment where the right side of (sub)expression is already a pointer/reference to T (or a child class of T).
    + or (sub)expression is a dynamic_cast<> 
      * NB: MOST of C-style casts, reinterpret_casts, and static_casts (formally - all those which cast between different classes) MUST be prohibited under generic [Rule 1], but SHOULD be reported separately under [Rule 1.1]
    + or (sub)expression is a function call
      * in practice, only unsafe functions can do it - but returning T* from owning_ptr<T>/soft_ptr<T>/naked_ptr<T> functions is necessary
    + or (sub)expression is nullptr
    + or (sub)expression is dereferencing of a raw pointer (T*), naked_ptr<T>, soft_ptr<T>, or owning_ptr<T>
    + NB: taking a variable address ("&i") is not necessary (it is done via constructor of naked_ptr<>)
    + TEST CASES/PROHIBIT: `(int*)p`, `p^p2`, `p+i`, `p[i]` (syntactic sugar for *(p+a) which is prohibited), `p1=p2=p+i`
    + TEST CASES/ALLOW: `dynamic_cast<X*>(p)`, `p=p2`, `p=np`, `p=sp`, `p=op`, `fp(p)`, `fp(np)`, `fp(sp)`, `fp(op)`, `&i`
  - **[Rule S1.1]** C-style casts, reinterpret_casts, and static_casts are prohibited. See NB in [Rule S1]. NB: this rule is NOT necessary to ensure safety, but significantly simplifies explaining and reporting.
* **[Rule S2]** dereferencing of raw X* pointers is prohibited (it is dereferencing of naked_ptr<T>, soft_ptr<T>, owning_ptr<T> which is allowed).
  + TEST CASES/PROHIBIT: `*nullptr`, `*p`
  + TEST CASES/ALLOW: `*np`, `*sp`, `*op`
  - **[Rule S2.1]** raw pointer variables (of type T*) are prohibited (necessary to ensure nullptr safety). Developers should use naked_ptr<> instead. NB: this rule is NOT necessary to ensure safety, but [Rule S1]+[Rule S2] makes such variables perfectly useless so it is better to prohibit them outright
    + NB: raw references are ok (we're ensuring that they're not null in the first place)
    + TEST CASES/PROHIBIT: `int* x;`
* **[Rule S3]** double pointers are prohibited, BOTH declared AND appearing implicitly within expressions. This also includes reference to a pointer.
  + TEST CASES/PROHIBIT: `int** x;`, `&p`, `int *& x = p;`, `void ff(int*& x)`
* **[Rule S4]** scope of raw pointer (T*) cannot expand
  + TEST CASES/PROHIBIT: return pointer to local variable, TODO (lots of them)
* **[Rule S5]** non-constant global variables, static variables, and thread_local variables are prohibited. NB: while prohibiting thread_local is not 100% required to ensure safety, it is still prohibited at least for now.
  + TEST CASES/PROHIBIT: `int x;` at global scope, `thread_local int x;`, `static int x;` within function, `static int x;` within the class
  + TEST CASES/ALLOW: `static void f();` within class, free-standing `static void f();`, `static constexpr int x;`
  + TODO/DECIDE: const statics (allowing them will require enforcing const-ness, including prohibiting mutables)
* **[Rule S6]** new operator is prohibited (developers should use make_owning<> instead)
  + TEST CASES/PROHIBIT: `new X()`, `new int`
  + TEST CASES/ALLOW: `make_owning<X>()`, `make_owning<int>()`
  - **[Rule S6.1]** result of make_owning<>() call MUST be assigned to an owning_ptr<T> (or passed to a function taking owning_ptr<T>) 
    + TEST CASES/PROHIBIT: `make_owning<X>();`, `soft_ptr<X> = make_owning<X>();`
    + TEST CASES/ALLOW: `auto x = make_owning<X>();`, `owning_ptr<X> x = make_owning<X>();`, `fop(make_owning<X>());`
  
### Determinism Checks

* Not allowing to convert pointers into non-pointers
  - **[Rule D1]** any (sub)expression which takes an argument of raw pointer type X* AND returns non-pointer type is prohibited, unless it is one of the following:
    + function call taking X* as parameter
    + TEST CASES/PROHIBIT: `(int)p`, `static_cast<int>(p)`
    + TEST CASES/ALLOW: `fp(p)`
    
