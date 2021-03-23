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

#ifndef SAFEMEMORY_SM_CONTAINERS_H
#define SAFEMEMORY_SM_CONTAINERS_H

/** \file
 * \brief Wrappers to easy switch between \c safememory containers and \c std ones
 * 
 */

#if defined(SAFEMEMORY_USE_SAFEMEMORY_CONTAINERS)

#include <safememory/vector.h>
#include <safememory/array.h>
#include <safememory/unordered_map.h>
#include <safememory/string.h>
#include <safememory/string_format.h>
#include <EASTL/utility.h>

namespace sm
{
	template<class T>
	using vector = safememory::vector<T>;

	template<class T, std::size_t N>
	using array = safememory::array<T, N>;

	using eastl::pair;
	using eastl::make_pair;

	template<class Key, class T, class Hash = safememory::hash<Key>, class Predicate = safememory::equal_to<Key>>
	using unordered_map = safememory::unordered_map<Key, T, Hash, Predicate>;

	template<class CharT>
	using basic_string = safememory::basic_string<CharT>;

	typedef basic_string<char>    string;
	typedef basic_string<wchar_t> wstring;

	using safememory::to_string;
	using safememory::to_wstring;

	/// ISO mandated string types
//	typedef basic_string<char8_t>  u8string; //enable if compiler supports char8_t
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;
}


#elif defined(SAFEMEMORY_USE_STD_CONTAINERS_WITH_IIBALLOCATOR)

//std containers with iibmalloc allocator

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <utility>
#include <safememory/detail/safe_ptr_common.h>

namespace sm
{
	using safememory::detail::iiballocator;

	template<class T>
	using vector = std::vector<T, iiballocator<T>>;

	template<class T, std::size_t N>
	using array = std::array<T, N>;

	using std::pair;
	using std::make_pair;

	template<class Key, class T, class Hash = std::hash<Key>, class Predicate = std::equal_to<Key>>
	using unordered_map = std::unordered_map<Key, T, Hash, Predicate, iiballocator<std::pair<const Key,T>>>;

	template<class CharT>
	using basic_string = std::basic_string<CharT, std::char_traits<CharT>, iiballocator<CharT>>;

	typedef basic_string<char>    string;
	typedef basic_string<wchar_t> wstring;

	// TODO must add functions for to_string
	// using std::to_string;
	// using std::to_wstring;

	/// ISO mandated string types
//	typedef basic_string<char8_t>  u8string; //enable if compiler supports char8_t
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;
}

#else
//just use std containers with std allocator

#include <string>
#include <vector>
#include <unordered_map>
#include <array>
#include <utility>

namespace sm
{
	template<class T>
	using vector = std::vector<T>;

	template<class T, std::size_t N>
	using array = std::array<T, N>;

	using std::pair;
	using std::make_pair;

	template<class Key, class T, class Hash = std::hash<Key>, class Predicate = std::equal_to<Key>>
	using unordered_map = std::unordered_map<Key, T, Hash, Predicate>;

	template<class T>
	using basic_string = std::basic_string<char>;

	typedef basic_string<char>    string;
	typedef basic_string<wchar_t> wstring;

	using std::to_string;
	using std::to_wstring;

	/// ISO mandated string types
//	typedef basic_string<char8_t>  u8string; //enable if compiler supports char8_t
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;
}

#endif

#endif // SAFEMEMORY_SM_CONTAINERS_H
