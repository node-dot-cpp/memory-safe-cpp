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

#include <safememory/detail/allocator_to_eastl.h>

namespace safememory::detail {

fixed_array_of<2, soft_ptr_with_zero_offset_impl<char>> gpSafeMemoryEmptyBucketArrayImpl = 
    { soft_ptr_with_zero_offset_impl<char>(), 
        soft_ptr_with_zero_offset_impl<char>(make_zero_offset_t(), hashtable_sentinel<char>())};

fixed_array_of<2, soft_ptr_with_zero_offset_no_checks<char>> gpSafeMemoryEmptyBucketArrayNoChecks = 
    { soft_ptr_with_zero_offset_no_checks<char>(), 
        soft_ptr_with_zero_offset_no_checks<char>(make_zero_offset_t(), hashtable_sentinel<char>())};

void* gpSafeMemoryEmptyBucketArrayRaw[] = { nullptr, hashtable_sentinel<void>()};

}

// required by eastl, see allocator.h line 173, or EASTL/doc/FAQ.md

void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return operator new [] (size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line) {
    return operator new [] (size, std::align_val_t(alignment));
}


