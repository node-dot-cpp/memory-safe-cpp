/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
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

#ifndef SAFE_PTR_COMMON_H
#define SAFE_PTR_COMMON_H

#include <cstdint>
#include <cstddef>
#include <utility>

#define NODECPP_ASSERT(...)

namespace safe_memory
{
	constexpr uint64_t module_id = 2;
}


#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif



namespace safe_memory
{
#ifdef NODECPP_GCC
extern void forcePreviousChangesToThisInDtor( void* p );
#else
#define forcePreviousChangesToThisInDtor(x)
#endif

template<class T>
void destruct( T* t )
{
}

struct make_owning_t {};

template<class T>
struct safeness_declarator {
	static constexpr bool is_safe = true; // by default
};

/* Sample of user-defined exclusion:
template<> struct safe_memory::safeness_declarator<double> { static constexpr bool is_safe = false; };
*/

// sample code (to be removed)
template<> struct safe_memory::safeness_declarator<double> { static constexpr bool is_safe = false; };
namespace testing::dummy_objects {
struct StructureWithSoftPtrDeclaredUnsafe; // forward declaration
}
template<> struct safe_memory::safeness_declarator<safe_memory::testing::dummy_objects::StructureWithSoftPtrDeclaredUnsafe> { static constexpr bool is_safe = false; }; // user-defined exclusion
// end of sample code (to be removed)

template<class T, bool isSafe> class owning_ptr_impl; // forward declaration
template<class T, bool isSafe> class soft_ptr_base_impl; // forward declaration
template<class T> class soft_ptr_base_no_checks; // forward declaration
template<class T> class soft_ptr_no_checks; // forward declaration




} // namespace safe_memory

#endif // SAFE_PTR_COMMON_H
