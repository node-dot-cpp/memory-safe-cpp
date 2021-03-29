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

#ifndef SAFE_MEMORY_STRING_LITERAL_H
#define SAFE_MEMORY_STRING_LITERAL_H

#include <safememory/memory_safety.h>
#include <safememory/detail/checker_attributes.h>
#include <safememory/detail/array_iterator.h>
#include <EASTL/iterator.h>
#include <EASTL/string_view.h>

namespace safememory
{
	template<typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFEMEMORY_DEEP_CONST SAFEMEMORY_NO_SIDE_EFFECT_WHEN_CONST basic_string_literal
	{
	public:
		typedef basic_string_literal<T, Safety>                           this_type;
		typedef T                                                         value_type;
		typedef const T&                                                  const_reference;
		typedef size_t                                                    size_type;
		// string literals have infinite lifetime, so raw pointer iterator is safe
		typedef detail::array_heap_safe_iterator<T, true, T*>	          const_heap_safe_iterator;
		typedef const_heap_safe_iterator                                  const_iterator_safe;
		typedef eastl::reverse_iterator<const_iterator_safe>              const_reverse_iterator_safe;

		static constexpr memory_safety is_safe = Safety;

	private:
		const value_type* str;
		size_type sz = 0;

		[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }
		// static size_type GetSize(const value_type* ptr) {
		// 	auto sz = eastl::CharStrlen(ptr);
		// 	// TODO add size check in case size_type is different of size_t
		// 	return static_cast<size_type>(sz); }

	public:
		
		template<size_t N>
		basic_string_literal(const value_type (&ptr)[N]) : str(ptr), sz(N - 1) { static_assert(N >= 1); }
		// basic_string_literal(const value_type* ptr, size_type n) : str(ptr), sz(n) {}

		basic_string_literal(const basic_string_literal& other) = default;
		basic_string_literal& operator=(const basic_string_literal& other) = default;
		basic_string_literal(basic_string_literal&& other) = default;
		basic_string_literal& operator=(basic_string_literal&& other) = default;

		constexpr const_iterator_safe begin() const noexcept { return const_iterator_safe::makeIx(str, 0, size()); }
		constexpr const_iterator_safe cbegin() const noexcept { return const_iterator_safe::makeIx(str, 0, size()); }

		constexpr const_iterator_safe end() const noexcept { return const_iterator_safe::makeIx(str, size(), size()); }
		constexpr const_iterator_safe cend() const noexcept { return const_iterator_safe::makeIx(str, size(), size()); }

		constexpr const_reverse_iterator_safe rbegin() const noexcept { return const_reverse_iterator_safe(cend()); }
		constexpr const_reverse_iterator_safe crbegin() const noexcept { return const_reverse_iterator_safe(cend()); }

		constexpr const_reverse_iterator_safe rend() const noexcept { return const_reverse_iterator_safe(cbegin()); }
		constexpr const_reverse_iterator_safe crend() const noexcept { return const_reverse_iterator_safe(cbegin()); }

		constexpr bool empty() const noexcept { return sz == 0; }
		constexpr size_type size() const noexcept { return sz; }

		constexpr const T* c_str() const noexcept { return str; }
		constexpr const T* data() const noexcept { return str; }

		// constexpr reference       operator[](size_type i);
		constexpr const_reference operator[](size_type i) const {
			return at(i);
		}

		constexpr const_reference at(size_type i) const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(i >= size()))
					ThrowRangeException("basic_string_literal::at -- out of range");
			}

			return str[i];
		}
		// constexpr reference       at(size_type i);

		constexpr const_reference front() const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("basic_string_literal::front -- empty vector");
			}

			return str[0];
		}

		constexpr const_reference back() const {
			if constexpr(is_safe == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty()))
					ThrowRangeException("basic_string_literal::back -- empty vector");
			}

			return str[size() - 1];
		}

		eastl::basic_string_view<T> to_string_view_unsafe() const {
			return eastl::basic_string_view<T>(data(), size());
		}
	};

	typedef basic_string_literal<char>    string_literal;
	typedef basic_string_literal<wchar_t> wstring_literal;

	/// string8 / string16 / string32
	typedef basic_string_literal<char8_t>  u8string_literal;
	typedef basic_string_literal<char16_t> u16string_literal;
	typedef basic_string_literal<char32_t> u32string_literal;


	/// user defined literals
// #if EASTL_USER_LITERALS_ENABLED && EASTL_INLINE_NAMESPACES_ENABLED
// 	inline namespace literals
// 	{
// 		inline namespace string_literals
// 		{
// 			inline string_literal operator"" _lit(const char* str, size_t len) EA_NOEXCEPT { return {str, string_literal::size_type(len)}; }
// 			inline u16string_literal operator"" _lit(const char16_t* str, size_t len) EA_NOEXCEPT { return {str, u16string_literal::size_type(len)}; }
// 			inline u32string_literal operator"" _lit(const char32_t* str, size_t len) EA_NOEXCEPT { return {str, u32string_literal::size_type(len)}; }
// 			inline wstring_literal operator"" _lit(const wchar_t* str, size_t len) EA_NOEXCEPT { return {str, wstring_literal::size_type(len)}; }

// 			// C++20 char8_t support.
// 			#if EA_CHAR8_UNIQUE
// 				inline u8string_literal operator"" _lit(const char8_t* str, size_t len) EA_NOEXCEPT { return {str, u8string_literal::size_type(len)}; }
// 			#endif
// 		}
// 	}
// #endif


	template<class T, memory_safety S>
	bool operator==( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator==(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator==( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator==(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator==( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator==(a.to_string_view_unsafe(), ptr);
	// }

	template<class T, memory_safety S>
	bool operator!=( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator!=(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator!=( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator!=(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator!=( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator!=(a.to_string_view_unsafe(), ptr);
	// }

	template<class T, memory_safety S>
	bool operator<( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator<(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator<( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator<(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator<( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator<(a.to_string_view_unsafe(), ptr);
	// }

	template<class T, memory_safety S>
	bool operator<=( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator<=(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator<=( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator<=(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator<=( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator<=(a.to_string_view_unsafe(), ptr);
	// }

	template<class T, memory_safety S>
	bool operator>( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator>(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator>( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator>(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator>( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator>(a.to_string_view_unsafe(), ptr);
	// }

	template<class T, memory_safety S>
	bool operator>=( const basic_string_literal<T, S>& a, const basic_string_literal<T, S>& b ) {
		return eastl::operator>=(a.to_string_view_unsafe(), b.to_string_view_unsafe());
	}

	// template<class T, memory_safety S>
	// bool operator>=( const typename basic_string_literal<T, S>::value_type* ptr, const basic_string_literal<T, S>& b ) {
	// 	return eastl::operator>=(ptr, b.to_string_view_unsafe());
	// }

	// template<class T, memory_safety S>
	// bool operator>=( const basic_string_literal<T, S>& a, const typename basic_string_literal<T, S>::value_type* ptr ) {
	// 	return eastl::operator>=(a.to_string_view_unsafe(), ptr);
	// }

} //namespace safememory

#endif //SAFE_MEMORY_STRING_LITERAL_H