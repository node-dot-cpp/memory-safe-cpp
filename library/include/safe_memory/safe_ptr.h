/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef SAFE_MEMORY_SAFE_PTR_H
#define SAFE_MEMORY_SAFE_PTR_H

//mb: temporary hack, until we move all files to their definitive location
// and rename namespaces acordingly

#include "../../src/safe_ptr.h"

namespace safe_memory {

using ::nodecpp::safememory::owning_ptr;
using ::nodecpp::safememory::soft_ptr;
using ::nodecpp::safememory::soft_this_ptr;
using ::nodecpp::safememory::nullable_ptr;

using ::nodecpp::safememory::make_owning;
using ::nodecpp::safememory::make_owning_2;
using ::nodecpp::safememory::soft_ptr_in_constructor;
using ::nodecpp::safememory::soft_ptr_static_cast;
using ::nodecpp::safememory::soft_ptr_reinterpret_cast;
using ::nodecpp::safememory::nullable_cast;

using ::nodecpp::safememory::make_owning_t;


using ::nodecpp::safememory::memory_safety;
using ::nodecpp::safememory::safeness_declarator;

}


#endif //SAFE_MEMORY_SAFE_PTR_H