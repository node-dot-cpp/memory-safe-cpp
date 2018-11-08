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
    + TEST CASES/PROHIBIT: `(int*)p`, `static_cast<int*>(p)`, reinterpret_cast<int*>(p)   
  - **[Rule S1.2]** Separate diagnostics for dereferencing of raw pointers (see above)
    + TEST CASES/PROHIBIT: `*nullptr`, `*p`
  - **[Rule S1.3]** raw pointer variables (of type T*) are prohibited; raw pointer function parameters are also prohibited. Developers should use naked_ptr<> instead. NB: this rule is NOT necessary to ensure safety, but [Rule S1] makes such variables perfectly useless (both calculating new values and dereferencing are prohibited on raw pointers) so it is better to prohibit them outright
    + NB: raw references are ok (we're ensuring that they're not null in the first place)
    + TEST CASES/PROHIBIT: `int* x;`
* **[Rule S2]** double pointers are prohibited, BOTH declared AND appearing implicitly within expressions. This also includes reference to a pointer.
  + TEST CASES/PROHIBIT: `int** x;`, `&p`, `int *& x = p;`, `void ff(int*& x)`
* **[Rule S3]** non-constant global variables, static variables, and thread_local variables are prohibited. NB: while prohibiting thread_local is not 100% required to ensure safety, it is still prohibited at least for now.
  + TEST CASES/PROHIBIT: `int x;` at global scope, `thread_local int x;`, `static int x;` within function, `static int x;` within the class
  + TEST CASES/ALLOW: `static void f();` within class, free-standing `static void f();`, `static constexpr int x;`
  + TODO/DECIDE: const statics (allowing them will require enforcing const-ness, including prohibiting mutables)
* **[Rule S4]** new operator is prohibited (developers should use make_owning<> instead)
  + TEST CASES/PROHIBIT: `new X()`, `new int`
  + TEST CASES/ALLOW: `make_owning<X>()`, `make_owning<int>()`
  - **[Rule S4.1]** result of make_owning<>() call MUST be assigned to an owning_ptr<T> (or passed to a function taking owning_ptr<T>) 
    + TEST CASES/PROHIBIT: `make_owning<X>();`, `soft_ptr<X> = make_owning<X>();`
    + TEST CASES/ALLOW: `auto x = make_owning<X>();`, `owning_ptr<X> x = make_owning<X>();`, `fop(make_owning<X>());`
* **[Rule S5]** scope of raw pointer (T*) cannot expand
  + TEST CASES/PROHIBIT: return pointer to local variable, TODO (lots of them)
  + **[Rule S5.1]** by default, no struct/class may contain a naked_ptr<> (neither a naked struct, neither even a pointer to a naked struct)
    - if a struct/class is marked up as `[[nodespp::naked_struct]]`, it may contain naked_ptrs (but not raw pointers)
      + naked_struct has its own scope similar to that of the naked pointers; normally , naked_struct requires that all the pointers within have the same scope, and then naked_struct has the same scope as all its pointers. [TODO: think about relaxing this requirement]
    - NB: having raw pointers (T*) is prohibited by [Rule S1.3]
  + **[Rule S5.2]** all functions are no-side-effect functions unless explicitly marked up
    - if a function without any markup accepts ONLY ONE naked_ptr<>, it is safe because there is no chance that this pointer is stored to any other parameter (it is not possible to store naked_ptr<> into a non-naked struct)
    - if a function without any markup accepts MORE THAN ONE naked_ptr<>, we have to assume that the scope of any of the naked-pointer parameters MAY extend to any other naked-pointer parameter.
      + In the future, we MAY introduce type-based analysis to relax this restriction
    - there is special parameter mark-up `[[nodecpp::may_extend_to_this]]`. It applies to parameters which are naked pointers or naked structs, AND means that the scope of marked-up parameter MAY be extended to 'this' of current function.
      + In the future, we MAY introduce other similar mark-up (`[[nodecpp:may_extend_to_a]]`?)
  + **[Rule S5.3]** Lambda is considered as an implicit naked struct, with the scope of lambda calculated as the NARROWEST of its captured references 
  
### Determinism Checks

* Not allowing to convert pointers into non-pointers
  - **[Rule D1]** any (sub)expression which takes an argument of raw pointer type X* AND returns non-pointer type is prohibited, unless it is one of the following:
    + function call taking X* as parameter
    + TEST CASES/PROHIBIT: `(int)p`, `static_cast<int>(p)`
    + TEST CASES/ALLOW: `fp(p)`
    
