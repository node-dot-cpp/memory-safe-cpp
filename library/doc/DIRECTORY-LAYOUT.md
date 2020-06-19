
As a general rule we are following Separate Header Placement from the [PFL](https://github.com/vector-of-bool/pitchfork/blob/spec/data/spec.bs), with the following details:

* Each sub project should have a single name (__PROJECT_NAME__) used as include prefix and namespace as detailed below.

* Code must have `.h/.ccp` pairs as components. If there is nothing to be put into .cpp, it should be present, containing nothing but a single include directive including its own `.h`; per __PFL__, this is a good practice to ensure that the list.hpp file will compile in isolation, ensuring that the header has the necessary `#include` directives and/or forward declarations.

* Public api headers that are intended to be directly included by the user go inside `include/PROJECT_NAME`. This makes the user include look like `#include <PROJECT_NAME/file.h>` making easy to identify where the file comes from, and avoids file name colisions. Header files that the user is not suposed to include directly shoul go in subfolder `include/PROJECT_NAME/detail`.

* When a component header has to be split into several files, suffix `_detail.h` may be used.

* All types and functions intended to be used directly by the user should be under a single namespace `PROJECT_NAME`. All types and functions not intended to be used by the user should go a subnamespace `PROJECT_NAME::detail`. This helps _IDE_ auto complete a lot, minimizing the number of irrelevant options shown to the user. Each sub project may alias (`using`) into its own namespace types and functions from other sub projects as needed. (i.e. `nodecpp::owning_ptr` as an alias of `safe_memory::owning_ptr`).

* Non public api headers go to `src` folder like mentioned at __PFL__. Non public header files should  have `_private.h` name sufix, to avoid potential for very messy mistakes.

* Submodules or subprojects should go to `external` (i.e. `external/iibmalloc` and `external/foundation` where appropiate).

* Plugins should go under `extras/plugins` and non-core modules under `extras/packages`

* One exception the rules - if we refactor 3rd-party stuff (such as _eastl_) then we put them into one folder, but within that folder we do keep original directory structure (including file names etc, etc,).


