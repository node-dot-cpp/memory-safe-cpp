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

#ifndef MEMORY_SAFETY_H
#define MEMORY_SAFETY_H

#include <cstdint>
//mb: temporary hack, until we move all files to their definitive location
// and rename namespaces acordingly

namespace safememory
{
	constexpr std::uint64_t module_id = 2;
} // namespace safememory

namespace safememory
{
	constexpr const char* safememory_module_id = "safememory";
}

namespace safememory {

enum class memory_safety { none, safe };

template<class T>
struct safeness_declarator {
#ifdef NODECPP_MEMORY_SAFETY
#if (NODECPP_MEMORY_SAFETY == 1) || (NODECPP_MEMORY_SAFETY == 0)
	static constexpr memory_safety is_safe = memory_safety::safe;
#elif NODECPP_MEMORY_SAFETY == -1
	static constexpr memory_safety is_safe = memory_safety::none;
#else
#error Unexpected value of NODECPP_MEMORY_SAFETY (expected values are -1, 1 or 0)
#endif // NODECPP_MEMORY_SAFETY defined
#else
	static constexpr memory_safety is_safe = memory_safety::safe; // by default
#endif
};

#ifdef NODECPP_MEMORY_SAFETY_EXCLUSIONS
#include NODECPP_MEMORY_SAFETY_EXCLUSIONS
#endif

/* Sample of user-defined exclusion:
template<> struct safememory::safeness_declarator<double> { static constexpr memory_safety is_safe = memory_safety::none; };
*/

} // namespace safememory


#endif //MEMORY_SAFETY_H