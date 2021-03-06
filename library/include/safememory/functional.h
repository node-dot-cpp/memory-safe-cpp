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

#ifndef SAFEMEMORY_FUNCTIONAL_H
#define SAFEMEMORY_FUNCTIONAL_H

#include <safememory/detail/checker_attributes.h>
#include <typeindex>

namespace SAFEMEMORY_CHECK_AS_USER_CODE safememory
{
	template<class T = void>
	struct SAFEMEMORY_DEEP_CONST equal_to {
		SAFEMEMORY_NO_SIDE_EFFECT constexpr bool operator()(const T &lhs, const T &rhs) const {
			return lhs == rhs;
		}
	};
}

	//mb: this has issues with [[no_side_effect]] analysis
	// template<>
	// class SAFEMEMORY_DEEP_CONST equal_to<void> {
	// 	template< class T, class U>
	// 	SAFEMEMORY_NO_SIDE_EFFECT constexpr auto operator()( T&& lhs, U&& rhs ) const
	// 		-> decltype(std::forward<T>(lhs) == std::forward<U>(rhs)) {
	// 			return lhs == rhs;
	// 		}
	// };

namespace safememory
{
	
	template <typename T> struct hash;

	template <typename T>
	struct SAFEMEMORY_DEEP_CONST hash : std::enable_if_t<std::is_enum_v<T>> {
		SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(T p) const { return std::size_t(p); }
	};

	template <>
	struct SAFEMEMORY_DEEP_CONST hash<std::type_index> {
		SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(const std::type_index& p) const { return p.hash_code(); }
	};

	template <typename T> struct SAFEMEMORY_DEEP_CONST hash<T*> // Note that we use the pointer as-is and don't divide by sizeof(T*). This is because the table is of a prime size and this division doesn't benefit distribution.
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(T* p) const { return reinterpret_cast<std::size_t>(p); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<bool>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(bool val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<char>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(char val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<signed char>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(signed char val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<unsigned char>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(unsigned char val) const { return static_cast<std::size_t>(val); } };

	template <> struct hash<char16_t>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(char16_t val) const { return static_cast<std::size_t>(val); } };

	template <> struct hash<char32_t>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(char32_t val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<wchar_t>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(wchar_t val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<signed short>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(signed short val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<unsigned short>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(unsigned short val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<signed int>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(signed int val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<unsigned int>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(unsigned int val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<signed long>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(signed long val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<unsigned long>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(unsigned long val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<signed long long>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(signed long long val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<unsigned long long>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(unsigned long long val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<float>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(float val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<double>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(double val) const { return static_cast<std::size_t>(val); } };

	template <> struct SAFEMEMORY_DEEP_CONST hash<long double>
		{ SAFEMEMORY_NO_SIDE_EFFECT std::size_t operator()(long double val) const { return static_cast<std::size_t>(val); } };

} //namespace safememory

#endif //SAFEMEMORY_FUNCTIONAL_H