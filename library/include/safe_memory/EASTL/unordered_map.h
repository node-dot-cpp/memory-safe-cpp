/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

// Initial vesion from:
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/unordered_map.h

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef SAFE_MEMORY_EASTL_UNORDERED_MAP_H
#define SAFE_MEMORY_EASTL_UNORDERED_MAP_H

#include <safe_memory/EASTL/hash_map.h>

namespace safe_memory
{
	/// unordered_map 
	///
	/// The original TR1 (technical report 1) used "hash_map" to name a hash
	/// table backed associative container of unique key-value pairs.  When the
	/// container was added to the C++11 standard the committee chose the name
	/// "unordered_map" to clarify that internally the elements are NOT sorted in
	/// any particular order.  We provide a template alias here to ensure feature
	/// parity with the original eastl::hash_map.
	///
	#if !defined(EA_COMPILER_NO_TEMPLATE_ALIASES)
		template <typename Key,
				  typename T,
				  typename Hash = hash<Key>,
				  typename Predicate = equal_to<Key>,
				  memory_safety Safety = memory_safety::safe,
				  bool bCacheHashCode = false>
		using unordered_map = detail::hash_map<Key, T, Hash, Predicate, Safety, bCacheHashCode>;
    #endif

    /// unordered_multimap
	///
	/// Similar template alias as "unordered_map" except the contained elements
	/// need not be unique. See "hash_multimap" for more details.
	///
	#if !defined(EA_COMPILER_NO_TEMPLATE_ALIASES)
		template <typename Key,
				  typename T,
				  typename Hash = hash<Key>,
				  typename Predicate = equal_to<Key>,
				  memory_safety Safety = memory_safety::safe,
				  bool bCacheHashCode = false>
		using unordered_multimap = detail::hash_multimap<Key, T, Hash, Predicate, Safety, bCacheHashCode>;
    #endif

} // namespace safe_memory

#endif // Header include guard
