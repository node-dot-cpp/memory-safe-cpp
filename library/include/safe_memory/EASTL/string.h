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
// https://github.com/electronicarts/EASTL/blob/3.15.00/include/EASTL/string.h

///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Implements a basic_string class, much like the C++ std::basic_string.
// The primary distinctions between basic_string and std::basic_string are:
//    - basic_string has a few extension functions that allow for increased performance.
//    - basic_string has a few extension functions that make use easier,
//      such as a member sprintf function and member tolower/toupper functions.
//    - basic_string is easier to read, debug, and visualize.
//    - basic_string has less deeply nested function calls and allows the user to
//      enable forced inlining in debug builds in order to reduce bloat.
//    - basic_string doesn't use char traits. As a result, EASTL assumes that
//      strings will hold characters and not exotic things like widgets. At the
//      very least, basic_string assumes that the value_type is a POD.
//    - basic_string data is guaranteed to be contiguous.
//    - basic_string data is guaranteed to be 0-terminated, and the c_str() function
//      is guaranteed to return the same pointer as the data() which is guaranteed
//      to be the same value as &string[0].
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Copy on Write (cow)
//
// This string implementation does not do copy on write (cow). This is by design,
// as cow penalizes 95% of string uses for the benefit of only 5% of the uses
// (these percentages are qualitative, not quantitative). The primary benefit of
// cow is that it allows for the sharing of string data between two string objects.
// Thus if you say this:
//    string a("hello");
//    string b(a);
// the "hello" will be shared between a and b. If you then say this:
//    a = "world";
// then a will release its reference to "hello" and leave b with the only reference
// to it. Normally this functionality is accomplished via reference counting and
// with atomic operations or mutexes.
//
// The C++ standard does not say anything about basic_string and cow. However,
// for a basic_string implementation to be standards-conforming, a number of
// issues arise which dictate some things about how one would have to implement
// a cow string. The discussion of these issues will not be rehashed here, as you
// can read the references below for better detail than can be provided in the
// space we have here. However, we can say that the C++ standard is sensible and
// that anything we try to do here to allow for an efficient cow implementation
// would result in a generally unacceptable string interface.
//
// The disadvantages of cow strings are:
//    - A reference count needs to exist with the string, which increases string memory usage.
//    - With thread safety, atomic operations and mutex locks are expensive, especially
//      on weaker memory systems such as console gaming platforms.
//    - All non-const string accessor functions need to do a sharing check the the
//      first such check needs to detach the string. Similarly, all string assignments
//      need to do a sharing check as well. If you access the string before doing an
//      assignment, the assignment doesn't result in a shared string, because the string
//      has already been detached.
//    - String sharing doesn't happen the large majority of the time. In some cases,
//      the total sum of the reference count memory can exceed any memory savings
//      gained by the strings that share representations.
//
// The addition of a string_cow class is under consideration for this library.
// There are conceivably some systems which have string usage patterns which would
// benefit from cow sharing. Such functionality is best saved for a separate string
// implementation so that the other string uses aren't penalized.
//
// References:
//    This is a good starting HTML reference on the topic:
//       http://www.gotw.ca/publications/optimizations.htm
//    Here is a Usenet discussion on the topic:
//       http://groups-beta.google.com/group/comp.lang.c++.moderated/browse_thread/thread/3dc6af5198d0bf7/886c8642cb06e03d
//
///////////////////////////////////////////////////////////////////////////////


#ifndef SAFE_MEMORY_EASTL_STRING_H
#define SAFE_MEMORY_EASTL_STRING_H

#include <safe_memory/EASTL/internal/__undef_macros.h>
#include <safe_memory/detail/safe_alloc.h>
#include <safe_memory/string_literal.h>
#include <safe_memory/functional.h>

#include <string>
#include <iterator>
#include <algorithm>
#include <initializer_list>
#include <climits>

//EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>             // size_t, ptrdiff_t, etc.
#include <stdarg.h>             // vararg functionality.

#include <stdlib.h>             // malloc, free.
#include <stdio.h>              // snprintf, etc.
#include <ctype.h>              // toupper, etc.

//EA_DISABLE_GCC_WARNING(-Wtype-limits)
#include <wchar.h>
//EA_RESTORE_GCC_WARNING()

#include <string.h> // strlen, etc.

// #if EASTL_EXCEPTIONS_ENABLED
	#include <stdexcept> // std::out_of_range, std::length_error.
// #endif
//EA_RESTORE_ALL_VC_WARNINGS()


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4530)  // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
	#pragma warning(disable: 4267)  // 'argument' : conversion from 'size_t' to 'const uint32_t', possible loss of data. This is a bogus warning resulting from a bug in VC++.
	#pragma warning(disable: 4480)  // nonstandard extension used: specifying underlying type for enum
	#pragma warning(disable: 4571)  // catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
	#pragma warning(disable: 4702)  // unreachable code
#endif


#include <safe_memory/EASTL/internal/char_traits.h>

namespace safe_memory
{
	///////////////////////////////////////////////////////////////////////////////
	/// basic_string
	///
	/// Implements a templated string class, somewhat like C++ std::basic_string.
	///
	/// Notes:
	///     As of this writing, an insert of a string into itself necessarily
	///     triggers a reallocation, even if there is enough capacity in self
	///     to handle the increase in size. This is due to the slightly tricky
	///     nature of the operation of modifying one's self with one's self,
	///     and thus the source and destination are being modified during the
	///     operation. It might be useful to rectify this to the extent possible.
	///
	///     Our usage of noexcept specifiers is a little different from the
	///     requirements specified by std::basic_string in C++11. This is because
	///     our allocators are instances and not types and thus can be non-equal
	///     and result in exceptions during assignments that theoretically can't
	///     occur with std containers.
	///
	

	template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFE_MEMORY_DEEP_CONST SAFE_MEMORY_NO_SIDE_EFFECT_WHEN_CONST basic_string
	{
	public:
		typedef basic_string<T, Safety>                      this_type;
		// typedef std::basic_string_view<T>                       view_type;
		typedef basic_string_literal<T>                       	literal_type;
		typedef T                                               value_type;
		typedef T*                                              pointer;
		typedef const T*                                        const_pointer;
		typedef T&                                              reference;
		typedef const T&                                        const_reference;
		// typedef std::reverse_iterator<pointer>                 reverse_iterator_unsafe;
		// typedef std::reverse_iterator<const_pointer>           const_reverse_iterator_unsafe;
		typedef std::size_t                                     size_type;
		typedef std::ptrdiff_t                                  difference_type;
		// typedef Allocator                                       allocator_type;

		typedef owning_ptr<detail::array_of2<T, Safety>, Safety> 				owning_heap_type;
		typedef detail::soft_ptr_with_zero_offset<detail::array_of2<T, Safety>, Safety> 	soft_heap_type;

		typedef detail::safe_array_iterator<T, Safety>				iterator;
		typedef detail::safe_array_iterator<const T, Safety>		const_iterator;
		typedef std::reverse_iterator<iterator>                	reverse_iterator;
		typedef std::reverse_iterator<const_iterator>          	const_reverse_iterator;
		typedef const const_iterator&							csafe_it_arg;
		typedef const const_reverse_iterator&					crsafe_it_arg;
		
		typedef std::pair<const_pointer, const_pointer>				const_pointer_pair;

		static const size_type npos     = static_cast<size_type>(-1);      /// 'npos' means non-valid position or simply non-position.
		static constexpr memory_safety is_safe = Safety;

	public:
		// CtorReserve exists so that we can create a constructor that allocates but doesn't
		// initialize and also doesn't collide with any other constructor declaration.
		struct CtorReserve{};

		// CtorSprintf exists so that we can create a constructor that accepts printf-style
		// arguments but also doesn't collide with any other constructor declaration.
//		struct CtorSprintf{};

		// CtorConvert exists so that we can have a constructor that implements string encoding
		// conversion, such as between UCS2 char16_t and UTF8 char8_t.
//		struct CtorConvert{};

	protected:
		static_assert(sizeof(size_type) >= sizeof(int), "Fix size!");
		static constexpr size_type kMaxSize = static_cast<size_type>(INT_MAX);

		static_assert(std::is_same<T, char>::value || std::is_same<T, wchar_t>::value ||
			std::is_same<T, char16_t>::value || std::is_same<T, char32_t>::value,
			"Type not supported!"); 

		struct Layout
		{
			std::size_t _size = 0;
			owning_heap_type _heap;


			Layout() { }
			Layout(const Layout& other) = delete;
			Layout(Layout&& other) = default;
			Layout& operator=(const Layout& other) = delete;
			Layout& operator=(Layout&& other) = default;

			inline size_type GetSize() const noexcept     { return _size; }

			inline void SetSize(size_type size) noexcept              { _size = size; }

			inline size_type GetRemainingCapacity() const noexcept    { return GetHeapCapacity() - _size; }

			inline pointer BeginPtr() noexcept                    { return _heap->begin(); }
			inline const_pointer BeginPtr() const noexcept        { return _heap->begin(); }

			// Points to end of character stream, *ptr == '0'
			inline pointer EndPtr() noexcept                      { return _heap->begin() + _size; }
			inline const_pointer EndPtr() const noexcept          { return _heap->begin() + _size; }

			// Points to end of the buffer at the terminating '0', *ptr == '0' <- only true when size() == capacity()
			inline pointer CapacityPtr() noexcept                 { return _heap->begin() + GetHeapCapacity(); }
			inline const_pointer CapacityPtr() const noexcept     { return _heap->begin() + GetHeapCapacity(); }

			inline void SetNewHeap(owning_heap_type&& new_heap) { _heap = std::move(new_heap); }
			inline soft_heap_type GetSoftHeapPtr() const noexcept        { return soft_heap_type(_heap); }

			inline size_type GetHeapCapacity() const noexcept { return _heap ? _heap->capacity() - 1 : 0; }
		};

		Layout          mPair_first;

		inline Layout& internalLayout() noexcept                        { return mPair_first; }
		inline const Layout& internalLayout() const noexcept            { return mPair_first; }
		inline soft_heap_type GetSoftHeapPtr() const noexcept        { return internalLayout().GetSoftHeapPtr(); }

	public:
		// Constructor, destructor
		basic_string() noexcept;
		// explicit basic_string(const allocator_type& allocator) noexcept;
		basic_string(const this_type& x, size_type position, size_type n = npos);
		basic_string(literal_type x);
		// basic_string(const_pointer p, size_type n/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
//		EASTL_STRING_EXPLICIT basic_string(const_pointer p/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
		basic_string(size_type n, value_type c/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
		basic_string(const this_type& x);
	    // basic_string(const this_type& x, const allocator_type& allocator);
		// basic_string(const_pointer pBegin, const_pointer pEnd/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
		basic_string(csafe_it_arg pBegin, csafe_it_arg pEnd);
		basic_string(CtorReserve, size_type n/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
//		basic_string(CtorSprintf, const_pointer pFormat, ...);
		basic_string(std::initializer_list<value_type> init/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);

		basic_string(this_type&& x) noexcept;
		// basic_string(this_type&& x, const allocator_type& allocator);

		// explicit basic_string(const view_type& sv/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);
		// basic_string(const view_type& sv, size_type position, size_type n/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);

		// template <typename OtherCharType>
		// basic_string(CtorConvert, const OtherCharType* p/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);

		// template <typename OtherCharType>
		// basic_string(CtorConvert, const OtherCharType* p, size_type n/*, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR*/);

		// template <typename OtherStringType> // Unfortunately we need the CtorConvert here because otherwise this function would collide with the pointer constructor.
		// basic_string(CtorConvert, const OtherStringType& x);

	   ~basic_string();

		// Allocator
		// const allocator_type& get_allocator() const noexcept;
		// allocator_type&       get_allocator() noexcept;
		// void                  set_allocator(const allocator_type& allocator);

		// Implicit conversion operator
		// operator std::basic_string_view<T>() const noexcept;

		// Operator=
		this_type& operator=(const this_type& x);
		this_type& operator=(literal_type x);
		// this_type& operator=(const_pointer p);
		this_type& operator=(value_type c);
		this_type& operator=(std::initializer_list<value_type> ilist);
		// this_type& operator=(view_type v);
		this_type& operator=(this_type&& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value);

		// #if EASTL_OPERATOR_EQUALS_OTHER_ENABLED
		// 	this_type& operator=(pointer p) { return operator=(const_cast<const_pointer>(p)); } // We need this because otherwise the const_pointer version can collide with the const OtherStringType& version below.

		// 	template <typename OtherCharType>
		// 	this_type& operator=(const OtherCharType* p);

		// 	template <typename OtherStringType>
		// 	this_type& operator=(const OtherStringType& x);
		// #endif

		void swap(this_type& x) noexcept;

		// Assignment operations
		this_type& assign(const this_type& x);
		this_type& assign(const this_type& x, size_type position, size_type n = npos);
		this_type& assign(literal_type x);
		this_type& assign_unsafe(const_pointer p, size_type n);
		this_type& assign_unsafe(const_pointer p);
		this_type& assign(size_type n, value_type c);
		this_type& assign_unsafe(const_pointer pBegin, const_pointer pEnd);
		this_type& assign(csafe_it_arg itBegin, csafe_it_arg itEnd);
		this_type& assign(this_type&& x); // TODO(c++17): noexcept(allocator_traits<Allocator>::propagate_on_container_move_assignment::value || allocator_traits<Allocator>::is_always_equal::value);
		this_type& assign(std::initializer_list<value_type>);

		template <typename OtherCharType>
		this_type& assign_convert_unsafe(const OtherCharType* p);

		template <typename OtherCharType>
		this_type& assign_convert_unsafe(const OtherCharType* p, size_type n);

		template <typename OtherCharType>
		this_type& assign_convert(const basic_string<OtherCharType>& x);

		template <typename OtherCharType>
		this_type& assign_convert(basic_string_literal<OtherCharType> p);

		// Iterators.
		pointer       begin_unsafe() noexcept;
		const_pointer begin_unsafe() const noexcept;
		const_pointer cbegin_unsafe() const noexcept;

		pointer       end_unsafe() noexcept;
		const_pointer end_unsafe() const noexcept;
		const_pointer cend_unsafe() const noexcept;

		// reverse_iterator_unsafe       rbegin_unsafe() noexcept;
		// const_reverse_iterator_unsafe rbegin_unsafe() const noexcept;
		// const_reverse_iterator_unsafe crbegin_unsafe() const noexcept;

		// reverse_iterator_unsafe       rend_unsafe() noexcept;
		// const_reverse_iterator_unsafe rend_unsafe() const noexcept;
		// const_reverse_iterator_unsafe crend_unsafe() const noexcept;

		iterator       begin() noexcept;
		const_iterator begin() const noexcept;
		const_iterator cbegin() const noexcept;

		iterator       end() noexcept;
		const_iterator end() const noexcept;
		const_iterator cend() const noexcept;

		reverse_iterator       rbegin() noexcept;
		const_reverse_iterator rbegin() const noexcept;
		const_reverse_iterator crbegin() const noexcept;

		reverse_iterator       rend() noexcept;
		const_reverse_iterator rend() const noexcept;
		const_reverse_iterator crend() const noexcept;


		// Size-related functionality
		bool      empty() const noexcept;
		size_type size() const noexcept;
		size_type length() const noexcept;
		size_type max_size() const noexcept;
		size_type capacity() const noexcept;
		void      resize(size_type n, value_type c);
		void      resize(size_type n);
		void      reserve(size_type = 0);
		void      set_capacity(size_type n = npos); // Revises the capacity to the user-specified value. Resizes the container to match the capacity if the requested capacity n is less than the current size. If n == npos then the capacity is reallocated (if necessary) such that capacity == size.
		void      force_size(size_type n);          // Unilaterally moves the string end position (mpEnd) to the given location. Useful for when the user writes into the string via some extenal means such as C strcpy or sprintf. This allows for more efficient use than using resize to achieve this.
		void 	  shrink_to_fit();

		// Raw access
		const_pointer data() const  noexcept;
		      pointer data()        noexcept;
		const_pointer c_str() const noexcept;

		// Element access
		reference       operator[](size_type n);
		const_reference operator[](size_type n) const;
		reference       at(size_type n);
		const_reference at(size_type n) const;
		reference       front();
		const_reference front() const;
		reference       back();
		const_reference back() const;

		// Append operations
		this_type& operator+=(const this_type& x);
		this_type& operator+=(literal_type p);
		// this_type& operator+=(const_pointer p);
		this_type& operator+=(value_type c);

		this_type& append(const this_type& x);
		this_type& append(const this_type& x,  size_type position, size_type n = npos);
		this_type& append(literal_type x);
		this_type& append_unsafe(const_pointer p, size_type n);
		this_type& append_unsafe(const_pointer p);
		this_type& append(size_type n, value_type c);
		this_type& append_unsafe(const_pointer pBegin, const_pointer pEnd);
		this_type& append(csafe_it_arg itBegin, csafe_it_arg itEnd);

		// this_type& append_sprintf_va_list(const_pointer pFormat, va_list arguments);
		// this_type& append_sprintf(const_pointer pFormat, ...);

		template <typename OtherCharType>
		this_type& append_convert_unsafe(const OtherCharType* p);

		template <typename OtherCharType>
		this_type& append_convert_unsafe(const OtherCharType* p, size_type n);

		template <typename OtherCharType>
		this_type& append_convert(const basic_string<OtherCharType>& x);

		template <typename OtherCharType>
		this_type& append_convert(basic_string_literal<OtherCharType> x);

		void push_back(value_type c);
		void pop_back();

		// Insertion operations
		this_type& insert(size_type position, const this_type& x);
		this_type& insert(size_type position, const this_type& x, size_type beg, size_type n);
		this_type& insert_unsafe(size_type position, const_pointer p, size_type n);
		this_type& insert_unsafe(size_type position, const_pointer p);
		this_type& insert(size_type position, literal_type x);
		this_type& insert(size_type position, size_type n, value_type c);
		pointer   insert_unsafe(const_pointer p, value_type c);
		pointer   insert_unsafe(const_pointer p, size_type n, value_type c);
		pointer   insert_unsafe(const_pointer p, const_pointer pBegin, const_pointer pEnd);
		pointer   insert_unsafe(const_pointer p, std::initializer_list<value_type>);
		iterator   insert(csafe_it_arg it, value_type c);
		iterator   insert(csafe_it_arg it, size_type n, value_type c);
		iterator   insert(csafe_it_arg it, csafe_it_arg itBegin, csafe_it_arg itEnd);
		iterator   insert(csafe_it_arg it, std::initializer_list<value_type>);

		// Erase operations
		this_type&       erase(size_type position = 0, size_type n = npos);
		pointer         erase_unsafe(const_pointer p);
		pointer         erase_unsafe(const_pointer pBegin, const_pointer pEnd);
		iterator    erase(csafe_it_arg it);
		iterator    erase(csafe_it_arg itBegin, csafe_it_arg itEnd);
		// reverse_iterator_unsafe erase_unsafe(reverse_iterator_unsafe position);
		// reverse_iterator_unsafe erase_unsafe(reverse_iterator_unsafe first, reverse_iterator_unsafe last);
		reverse_iterator erase(crsafe_it_arg position);
		reverse_iterator erase(crsafe_it_arg first, crsafe_it_arg last);
		void             clear() noexcept;

		// Detach memory
		// pointer detach() noexcept;

		// Replacement operations
		this_type&  replace(size_type position, size_type n,  const this_type& x);
		this_type&  replace(size_type pos1,     size_type n1, const this_type& x,  size_type pos2, size_type n2 = npos);
		this_type&  replace_unsafe(size_type position, size_type n1, const_pointer p, size_type n2);
		this_type&  replace_unsafe(size_type position, size_type n1, const_pointer p);
		this_type&  replace(size_type position, size_type n1, literal_type x);
		this_type&  replace(size_type position, size_type n1, size_type n2, value_type c);
		this_type&  replace_unsafe(const_pointer first, const_pointer last, const this_type& x);
		this_type&  replace_unsafe(const_pointer first, const_pointer last, const_pointer p, size_type n);
		this_type&  replace_unsafe(const_pointer first, const_pointer last, const_pointer p);
//		this_type&  replace_unsafe(const_iterator first, const_iterator last, literal_type x);
		this_type&  replace_unsafe(const_pointer first, const_pointer last, size_type n, value_type c);
		this_type&  replace_unsafe(const_pointer first, const_pointer last, const_pointer pBegin, const_pointer pEnd);

		this_type&  replace(csafe_it_arg first, csafe_it_arg last, const this_type& x);
//		this_type&  replace(csafe_it_arg first, csafe_it_arg last, const_pointer p, size_type n);
//		this_type&  replace(const_iterator first, const_iterator last, const_pointer p);
		this_type&  replace(csafe_it_arg first, csafe_it_arg last, literal_type x);
		this_type&  replace(csafe_it_arg first, csafe_it_arg last, size_type n, value_type c);
		this_type&  replace(csafe_it_arg first, csafe_it_arg last, csafe_it_arg itBegin, csafe_it_arg itEnd);

		size_type   copy_unsafe(pointer p, size_type n, size_type position = 0) const;

		// Find operations
		size_type find(const this_type& x,  size_type position = 0) const noexcept;
		size_type find_unsafe(const_pointer p, size_type position = 0) const;
		size_type find(literal_type x, size_type position = 0) const;
		size_type find_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type find(value_type c, size_type position = 0) const noexcept;

		// Reverse find operations
		size_type rfind(const this_type& x,  size_type position = npos) const noexcept;
		size_type rfind_unsafe(const_pointer p, size_type position = npos) const;
		size_type rfind(literal_type x, size_type position = npos) const;
		size_type rfind_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type rfind(value_type c, size_type position = npos) const noexcept;

		// Find first-of operations
		size_type find_first_of(const this_type& x, size_type position = 0) const noexcept;
		size_type find_first_of_unsafe(const_pointer p, size_type position = 0) const;
		size_type find_first_of(literal_type x, size_type position = 0) const;
		size_type find_first_of_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type find_first_of(value_type c, size_type position = 0) const noexcept;

		// Find last-of operations
		size_type find_last_of(const this_type& x, size_type position = npos) const noexcept;
		size_type find_last_of_unsafe(const_pointer p, size_type position = npos) const;
		size_type find_last_of(literal_type x, size_type position = npos) const;
		size_type find_last_of_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type find_last_of(value_type c, size_type position = npos) const noexcept;

		// Find first not-of operations
		size_type find_first_not_of(const this_type& x, size_type position = 0) const noexcept;
		size_type find_first_not_of_unsafe(const_pointer p, size_type position = 0) const;
		size_type find_first_not_of(literal_type x, size_type position = 0) const;
		size_type find_first_not_of_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type find_first_not_of(value_type c, size_type position = 0) const noexcept;

		// Find last not-of operations
		size_type find_last_not_of(const this_type& x,  size_type position = npos) const noexcept;
		size_type find_last_not_of_unsafe(const_pointer p, size_type position = npos) const;
		size_type find_last_not_of(literal_type x, size_type position = npos) const;
		size_type find_last_not_of_unsafe(const_pointer p, size_type position, size_type n) const;
		size_type find_last_not_of(value_type c, size_type position = npos) const noexcept;

		// Substring functionality
		this_type substr(size_type position = 0, size_type n = npos) const;

		// Comparison operations
		int        compare(const this_type& x) const noexcept;
		int        compare(size_type pos1, size_type n1, const this_type& x) const;
		int        compare(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2) const;
		int        compare_unsafe(const_pointer p) const;
		int        compare(literal_type x) const;
		int        compare_unsafe(size_type pos1, size_type n1, const_pointer p) const;
		int        compare(size_type pos1, size_type n1, literal_type x) const;
		int        compare_unsafe(size_type pos1, size_type n1, const_pointer p, size_type n2) const;
		static int compare_unsafe(const_pointer pBegin1, const_pointer pEnd1, const_pointer pBegin2, const_pointer pEnd2);
		static int compare(csafe_it_arg itBegin1, csafe_it_arg itEnd1, csafe_it_arg itBegin2, csafe_it_arg itEnd2);

		// Case-insensitive comparison functions. Not part of C++ this_type. Only ASCII-level locale functionality is supported. Thus this is not suitable for localization purposes.
		// int        comparei(const this_type& x) const noexcept;
		// int        comparei(const_pointer p) const;
		// static int comparei(const_pointer pBegin1, const_pointer pEnd1, const_pointer pBegin2, const_pointer pEnd2);

		// Misc functionality, not part of C++ this_type.
		void         make_lower();
		void         make_upper();
		void         ltrim();
		void         rtrim();
		void         trim();
		void         ltrim_unsafe(const_pointer p);
		void         ltrim(literal_type x);
		void         rtrim_unsafe(const_pointer p);
		void         rtrim(literal_type x);
		void         trim_unsafe(const_pointer p);
		void         trim(literal_type x);
		this_type    left(size_type n) const;
		this_type    right(size_type n) const;
		// this_type&   sprintf_va_list(const_pointer pFormat, va_list arguments);
		// this_type&   sprintf(const_pointer pFormat, ...);

		std::size_t hash() const noexcept;

		bool validate() const noexcept;
		detail::iterator_validity  validate_iterator(const_pointer i) const noexcept;
		detail::iterator_validity  validate_iterator(csafe_it_arg i) const noexcept;

	protected:
		// Helper functions for initialization/insertion operations.

		owning_heap_type DoAllocate(size_type n);
		// void        DoFree(pointer p, size_type n);
		static size_type   GetNewCapacity(size_type currentCapacity, size_type minimumRequiredCapacity);
		void        AllocateSelf();
		void        AllocateSelf(size_type n);
		// void        DeallocateSelf();
		pointer    InsertInternal(const_pointer p, value_type c);
		void        RangeInitialize(const_pointer pBegin, const_pointer pEnd);
		void        RangeInitialize(const_pointer pBegin);
		void        SizeInitialize(size_type n, value_type c);

		// bool        IsSSO() const noexcept;

		[[noreturn]] static void ThrowLengthException(const char* msg) { throw std::length_error(msg); }
		[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }
		[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw std::invalid_argument(msg); }
		[[noreturn]] static void ThrowMaxSizeException(const char* msg) { throw std::out_of_range(msg); }
		

		static
		const_pointer_pair checkAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd);
		
		const_pointer checkMineAndGet(csafe_it_arg it) const;
		const_pointer_pair checkMineAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd) const;

		static
		const_pointer_pair toPtrPair(literal_type x);
		static
		const_pointer_pair toPtrPair(const this_type& str, size_type pos, size_type n);
	
	public:
		template <typename Arg>
		static
		this_type snprintf_unsafe(const char* fmt, Arg arg) {
			int sz = std::snprintf(nullptr, 0, fmt, arg);
			this_type buffer;
			if(sz < 0)
				return buffer;
			
			buffer.resize(static_cast<std::size_t>(sz));
			// snprintf will write ending '\0'
			std::snprintf(buffer.data(), buffer.size() + 1, fmt, arg);
			return buffer;
		};

		template <typename Arg>
		static
		this_type swprintf_unsafe(const wchar_t* fmt, Arg arg) {
			int sz = std::swprintf(nullptr, 0, fmt, arg);
			this_type buffer;
			if(sz < 0)
				return buffer;
			
			buffer.resize(static_cast<std::size_t>(sz));
			// snprintf will write ending '\0'
			std::swprintf(buffer.data(), buffer.size() + 1, fmt, arg);
			return buffer;
		};

		// #if EASTL_OPERATOR_EQUALS_OTHER_ENABLED
		// 	template <typename CharType>
		// 	void DoAssignConvert(CharType c, true_type);

		// 	template <typename StringType>
		// 	void DoAssignConvert(const StringType& x, false_type);
		// #endif

		// Replacements for STL template functions.
		static const_pointer CharTypeStringFindEnd(const_pointer pBegin, const_pointer pEnd, value_type c);
		static const_pointer CharTypeStringRFind(const_pointer pRBegin, const_pointer pREnd, const value_type c);
		static const_pointer CharTypeStringSearch(const_pointer p1Begin, const_pointer p1End, const_pointer p2Begin, const_pointer p2End);
		static const_pointer CharTypeStringRSearch(const_pointer p1Begin, const_pointer p1End, const_pointer p2Begin, const_pointer p2End);
		static const_pointer CharTypeStringFindFirstOf(const_pointer p1Begin, const_pointer p1End, const_pointer p2Begin, const_pointer p2End);
		static const_pointer CharTypeStringRFindFirstOf(const_pointer p1RBegin, const_pointer p1REnd, const_pointer p2Begin, const_pointer p2End);
		static const_pointer CharTypeStringFindFirstNotOf(const_pointer p1Begin, const_pointer p1End, const_pointer p2Begin, const_pointer p2End);
		static const_pointer CharTypeStringRFindFirstNotOf(const_pointer p1RBegin, const_pointer p1REnd, const_pointer p2Begin, const_pointer p2End);

	}; // basic_string





	///////////////////////////////////////////////////////////////////////////////
	// basic_string
	///////////////////////////////////////////////////////////////////////////////

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::basic_string() noexcept
	    /*: mPair_second(allocator_type(EASTL_BASIC_STRING_DEFAULT_NAME))*/
	{
		AllocateSelf();
	}


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const allocator_type& allocator) noexcept
	//     : mPair_second(allocator)
	// {
	// 	AllocateSelf();
	// }


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::basic_string(const this_type& x)
	    // : mPair_second(x.get_allocator())
	{
		RangeInitialize(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	// template <typename T, memory_safety Safety>
	// basic_string<T, Safety>::basic_string(const this_type& x, const allocator_type& allocator)
	// 	: mPair_second(allocator)
	// {
	// 	RangeInitialize(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	// }


	// template <typename T, memory_safety Safety>
	// template <typename OtherStringType>
	// inline basic_string<T, Safety>::basic_string(CtorConvert, const OtherStringType& x)
	//     // : mPair_second(x.get_allocator())
	// {
	// 	AllocateSelf();
	// 	append_convert(x.c_str(), x.length());
	// }


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>::basic_string(const this_type& x, size_type position, size_type n)
		// : mPair_second(x.get_allocator())
	{
		const_pointer_pair p = toPtrPair(x, position, n);
		RangeInitialize(p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::basic_string(literal_type x)
	{
		RangeInitialize(x.c_str());
	}

	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const_pointer p, size_type n/*, const allocator_type& allocator*/)
	// 	// : mPair_second(allocator)
	// {
	// 	RangeInitialize(p, p + n);
	// }


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const view_type& sv/*, const allocator_type& allocator*/)
	//     : basic_string(sv.data(), sv.size()/*, allocator*/)
	// {
	// }


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const view_type& sv, size_type position, size_type n/*, const allocator_type& allocator*/)
	//     : basic_string(sv.substr(position, n)/*, allocator*/)
	// {
	// }


	// template <typename T, memory_safety Safety>
	// template <typename OtherCharType>
	// inline basic_string<T, Safety>::basic_string(CtorConvert, const OtherCharType* p/*, const allocator_type& allocator*/)
	// 	// : mPair_second(allocator)
	// {
	// 	AllocateSelf();    // In this case we are converting from one string encoding to another, and we
	// 	append_convert(p); // implement this in the simplest way, by simply default-constructing and calling assign.
	// }


	// template <typename T, memory_safety Safety>
	// template <typename OtherCharType>
	// inline basic_string<T, Safety>::basic_string(CtorConvert, const OtherCharType* p, size_type n/*, const allocator_type& allocator*/)
	// 	// : mPair_second(allocator)
	// {
	// 	AllocateSelf();         // In this case we are converting from one string encoding to another, and we
	// 	append_convert(p, n);   // implement this in the simplest way, by simply default-constructing and calling assign.
	// }


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const_pointer p/*, const allocator_type& allocator*/)
	// 	// : mPair_second(allocator)
	// {
	// 	RangeInitialize(p);
	// }


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::basic_string(size_type n, value_type c/*, const allocator_type& allocator*/)
		// : mPair_second(allocator)
	{
		SizeInitialize(n, c);
	}


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>::basic_string(const_pointer pBegin, const_pointer pEnd/*, const allocator_type& allocator*/)
	// 	// : mPair_second(allocator)
	// {
	// 	RangeInitialize(pBegin, pEnd);
	// }

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::basic_string(csafe_it_arg itBegin, csafe_it_arg itEnd)
		// : mPair_second(allocator)
	{
		const_pointer_pair p = checkAndGet(itBegin, itEnd);
		RangeInitialize(p.first, p.second);
	}


	// CtorReserve exists so that we can create a version that allocates but doesn't
	// initialize but also doesn't collide with any other constructor declaration.
	template <typename T, memory_safety Safety>
	basic_string<T, Safety>::basic_string(CtorReserve /*unused*/, size_type n/*, const allocator_type& allocator*/)
		// : mPair_second(allocator)
	{
		// Note that we do not call SizeInitialize here.
		AllocateSelf(n);
		*internalLayout().EndPtr() = 0;
	}


	// // CtorSprintf exists so that we can create a version that does a variable argument
	// // sprintf but also doesn't collide with any other constructor declaration.
	// template <typename T, memory_safety Safety>
	// basic_string<T, Safety>::basic_string(CtorSprintf /*unused*/, const_pointer pFormat, ...)
	// 	// : mPair_second()
	// {
	// 	const size_type n = (size_type)CharStrlen(pFormat);
	// 	AllocateSelf(n);

	// 	va_list arguments;
	// 	va_start(arguments, pFormat);
	// 	append_sprintf_va_list(pFormat, arguments);
	// 	va_end(arguments);
	// }


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>::basic_string(std::initializer_list<value_type> init/*, const allocator_type& allocator*/)
		// : mPair_second(allocator)
	{
		RangeInitialize(init.begin(), init.end());
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>::basic_string(this_type&& x) noexcept
		// : mPair_second(x.get_allocator())
	{
		// internalLayout() = std::move(x.internalLayout());
		// x.AllocateSelf();
		swap(x);
	}


	// template <typename T, memory_safety Safety>
	// basic_string<T, Safety>::basic_string(this_type&& x, const allocator_type& allocator)
	// : mPair_second(allocator)
	// {
	// 	if(get_allocator() == x.get_allocator()) // If we can borrow from x...
	// 	{
	// 		internalLayout() = std::move(x.internalLayout());
	// 		x.AllocateSelf();
	// 	}
	// 	else if(x.internalLayout().BeginPtr())
	// 	{
	// 		RangeInitialize(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	// 		// Let x destruct its own items.
	// 	}
	// }


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>::~basic_string()
	{
		// DeallocateSelf();
	}


	// template <typename T, memory_safety Safety>
	// inline const typename basic_string<T, Safety>::allocator_type&
	// basic_string<T, Safety>::get_allocator() const noexcept
	// {
	// 	return internalAllocator();
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::allocator_type&
	// basic_string<T, Safety>::get_allocator() noexcept
	// {
	// 	return internalAllocator();
	// }


	// template <typename T, memory_safety Safety>
	// inline void basic_string<T, Safety>::set_allocator(const allocator_type& allocator)
	// {
	// 	get_allocator() = allocator;
	// }


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::data()  const noexcept
	{
		return internalLayout().BeginPtr();
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::c_str() const noexcept
	{
		return internalLayout().BeginPtr();
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::data() noexcept
	{
		return internalLayout().BeginPtr();
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::begin_unsafe() noexcept
	{
		return pointer(internalLayout().BeginPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::end_unsafe() noexcept
	{
		return pointer(internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::begin_unsafe() const noexcept
	{
		return const_pointer(internalLayout().BeginPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::cbegin_unsafe() const noexcept
	{
		return const_pointer(internalLayout().BeginPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::end_unsafe() const noexcept
	{
		return const_pointer(internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::cend_unsafe() const noexcept
	{
		return const_pointer(internalLayout().EndPtr());
	}


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::reverse_iterator_unsafe
	// basic_string<T, Safety>::rbegin_unsafe() noexcept
	// {
	// 	return reverse_iterator_unsafe(end_unsafe());
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::reverse_iterator_unsafe
	// basic_string<T, Safety>::rend_unsafe() noexcept
	// {
	// 	return reverse_iterator_unsafe(begin_unsafe());
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::const_reverse_iterator_unsafe
	// basic_string<T, Safety>::rbegin_unsafe() const noexcept
	// {
	// 	return const_reverse_iterator_unsafe(end_unsafe());
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::const_reverse_iterator_unsafe
	// basic_string<T, Safety>::crbegin_unsafe() const noexcept
	// {
	// 	return const_reverse_iterator_unsafe(end_unsafe());
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::const_reverse_iterator_unsafe
	// basic_string<T, Safety>::rend_unsafe() const noexcept
	// {
	// 	return const_reverse_iterator_unsafe(begin_unsafe());
	// }


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::const_reverse_iterator_unsafe
	// basic_string<T, Safety>::crend_unsafe() const noexcept
	// {
	// 	return const_reverse_iterator_unsafe(begin_unsafe());
	// }

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::begin() noexcept
	{
		return iterator::make(GetSoftHeapPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::end() noexcept
	{
		return iterator::make(GetSoftHeapPtr(), size());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_iterator
	basic_string<T, Safety>::begin() const noexcept
	{
		return const_iterator::make(GetSoftHeapPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_iterator
	basic_string<T, Safety>::cbegin() const noexcept
	{
		return const_iterator::make(GetSoftHeapPtr());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_iterator
	basic_string<T, Safety>::end() const noexcept
	{
		return const_iterator::make(GetSoftHeapPtr(), size());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_iterator
	basic_string<T, Safety>::cend() const noexcept
	{
		return const_iterator::make(GetSoftHeapPtr(), size());
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reverse_iterator
	basic_string<T, Safety>::rbegin() noexcept
	{
		return reverse_iterator(end());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reverse_iterator
	basic_string<T, Safety>::rend() noexcept
	{
		return reverse_iterator(begin());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reverse_iterator
	basic_string<T, Safety>::rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reverse_iterator
	basic_string<T, Safety>::crbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reverse_iterator
	basic_string<T, Safety>::rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reverse_iterator
	basic_string<T, Safety>::crend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

	template <typename T, memory_safety Safety>
	inline bool basic_string<T, Safety>::empty() const noexcept
	{
		return (size() == 0);
	}


	// template <typename T, memory_safety Safety>
	// inline bool basic_string<T, Safety>::IsSSO() const noexcept
	// {
	// 	return internalLayout().IsSSO();
	// }


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::size() const noexcept
	{
		return internalLayout().GetSize();
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::length() const noexcept
	{
		return internalLayout().GetSize();
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::max_size() const noexcept
	{
		return kMaxSize;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::capacity() const noexcept
	{
		// if (internalLayout().IsHeap())
		// {
			return internalLayout().GetHeapCapacity();
		// }
		// return SSOLayout::SSO_CAPACITY;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reference
	basic_string<T, Safety>::operator[](size_type n) const
	{
		return at(n);
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reference
	basic_string<T, Safety>::operator[](size_type n)
	{
		return at(n);
	}


	// template <typename T, memory_safety Safety>
	// basic_string<T,Allocator>::operator std::basic_string_view<T>() const noexcept
	// {
	// 	return std::basic_string_view<T>(data(), size());
	// }


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(const this_type& x)
	{
		if(&x != this)
		{
			// #if EASTL_ALLOCATOR_COPY_ENABLED
			// 	bool bSlowerPathwayRequired = (get_allocator() != x.get_allocator());
			// #else
			// 	bool bSlowerPathwayRequired = false;
			// #endif

			// if(bSlowerPathwayRequired)
			// {
			// 	set_capacity(0); // Must use set_capacity instead of clear because set_capacity frees our memory, unlike clear.

			// 	#if EASTL_ALLOCATOR_COPY_ENABLED
			// 		get_allocator() = x.get_allocator();
			// 	#endif
			// }

			assign_unsafe(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
		}
		return *this;
	}


	// #if EASTL_OPERATOR_EQUALS_OTHER_ENABLED
	// 	template <typename T, memory_safety Safety>
	// 	template <typename CharType>
	// 	inline void basic_string<T, Safety>::DoAssignConvert(CharType c, true_type)
	// 	{
	// 		assign_convert(&c, 1); // Call this version of append because it will result in the encoding-converting append being used.
	// 	}


	// 	template <typename T, memory_safety Safety>
	// 	template <typename StringType>
	// 	inline void basic_string<T, Safety>::DoAssignConvert(const StringType& x, false_type)
	// 	{
	// 		//if(&x != this) // Unnecessary because &x cannot possibly equal this.
	// 		{
	// 			#if EASTL_ALLOCATOR_COPY_ENABLED
	// 				get_allocator() = x.get_allocator();
	// 			#endif

	// 			assign_convert(x.c_str(), x.length());
	// 		}
	// 	}


	// 	template <typename T, memory_safety Safety>
	// 	template <typename OtherStringType>
	// 	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(const OtherStringType& x)
	// 	{
	// 		clear();
	// 		DoAssignConvert(x, is_integral<OtherStringType>());
	// 		return *this;
	// 	}


	// 	template <typename T, memory_safety Safety>
	// 	template <typename OtherCharType>
	// 	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(const OtherCharType* p)
	// 	{
	// 		return assign_convert(p);
	// 	}
	// #endif

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(literal_type x)
	{
		const T* p = x.c_str();
		return assign_unsafe(p, p + CharStrlen(p));
	}

	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(const_pointer p)
	// {
	// 	return assign(p, p + CharStrlen(p));
	// }

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(value_type c)
	{
		return assign(static_cast<size_type>(1), c);
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(this_type&& x)
	{
		return assign(std::move(x));
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(std::initializer_list<value_type> ilist)
	{
		return assign_unsafe(ilist.begin(), ilist.end());
	}


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::this_type& basic_string<T, Safety>::operator=(view_type v)
	// {
	// 	return assign(v.data(), static_cast<this_type::size_type>(v.size()));
	// }


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::resize(size_type n, value_type c)
	{
		const size_type s = internalLayout().GetSize();

		if(n < s)
			erase_unsafe(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
		else if(n > s)
			append(n - s, c);
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::resize(size_type n)
	{
		// C++ basic_string specifies that resize(n) is equivalent to resize(n, value_type()).
		// For built-in types, value_type() is the same as zero (value_type(0)).
		// We can improve the efficiency (especially for long strings) of this
		// string class by resizing without assigning to anything.

		const size_type s = internalLayout().GetSize();

		if(n < s)
			erase_unsafe(begin_unsafe() + n, end_unsafe());
		else if(n > s)
		{
			append(n - s, value_type());
		}
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::reserve(size_type n)
	{
		if(NODECPP_UNLIKELY(n > max_size()))
			ThrowLengthException("basic_string::reserve -- n > max_size()");

		// C++20 says if the passed in capacity is less than the current capacity we do not shrink
		// If new_cap is less than or equal to the current capacity(), there is no effect.
		// http://en.cppreference.com/w/cpp/string/basic_string/reserve

		n = std::max(n, internalLayout().GetSize()); // Calculate the new capacity, which needs to be >= container size.

		if(n > capacity())
			set_capacity(n);
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::shrink_to_fit()
	{
		set_capacity(internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::set_capacity(size_type n)
	{
		if(n == npos)
			// If the user wants to set the capacity to equal the current size...
			// '-1' because we pretend that we didn't allocate memory for the terminating 0.
			n = internalLayout().GetSize();
		else if(n < internalLayout().GetSize())
		{
			internalLayout().SetSize(n);
			*internalLayout().EndPtr() = 0;
		}

		if((n < capacity() /*&& internalLayout().IsHeap()*/) || (n > capacity()))
		{
			// In here the string is transition from heap->heap, heap->sso or sso->heap

			if(NODECPP_LIKELY(n))
			{

				// if(n <= SSOLayout::SSO_CAPACITY)
				// {
				// 	// heap->sso
				// 	// A heap based layout wants to reduce its size to within sso capacity
				// 	// An sso layout wanting to reduce its capacity will not get in here
				// 	pointer pOldBegin = internalLayout().BeginPtr();
				// 	const size_type nOldCap = internalLayout().GetHeapCapacity();

				// 	internalLayout().ResetToSSO(); // reset layout to sso
				// 	CharStringUninitializedCopy(pOldBegin, pOldBegin + n, internalLayout().BeginPtr());
				// 	// *EndPtr() = 0 is already done by the ResetToSSO
				// 	internalLayout().SetSSOSize(n);

				// 	DoFree(pOldBegin, nOldCap + 1);

				// 	return;
				// }

				owning_heap_type pNewBegin = DoAllocate(n + 1); // We need the + 1 to accomodate the trailing 0.
				size_type nSavedSize = internalLayout().GetSize(); // save the size in case we transition from sso->heap

				pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pNewBegin->begin());
				*pNewEnd = 0;

				// DeallocateSelf();

				internalLayout().SetNewHeap(std::move(pNewBegin));
//				internalLayout().SetHeapCapacity(n);
				internalLayout().SetSize(nSavedSize);
			}
			else
			{
				// DeallocateSelf();
				AllocateSelf();
			}
		}
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::force_size(size_type n)
	{
		if(NODECPP_UNLIKELY(n > capacity()))
			ThrowRangeException("basic_string -- out of range");

		internalLayout().SetSize(n);
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::clear() noexcept
	{
		internalLayout().SetSize(0);
		*internalLayout().BeginPtr() = value_type(0);
	}


	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::pointer
	// basic_string<T, Safety>::detach() noexcept
	// {
	// 	// The detach function is an extension function which simply forgets the
	// 	// owned pointer. It doesn't free it but rather assumes that the user
	// 	// does. If the string is utilizing the short-string-optimization when a
	// 	// detach is requested, a copy of the string into a seperate memory
	// 	// allocation occurs and the owning pointer is given to the user who is
	// 	// responsible for freeing the memory.

	// 	pointer pDetached = nullptr;

	// 	if (internalLayout().IsSSO())
	// 	{
	// 		const size_type n = internalLayout().GetSize() + 1; // +1' so that we have room for the terminating 0.
	// 		pDetached = DoAllocate(n);
	// 		pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pDetached);
	// 		*pNewEnd = 0;
	// 	}
	// 	else
	// 	{
	// 		pDetached = internalLayout().BeginPtr();
	// 	}

	// 	AllocateSelf(); // reset to string to empty
	// 	return pDetached;
	// }


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reference
	basic_string<T, Safety>::at(size_type n) const
	{
		if(NODECPP_UNLIKELY(n >= internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		return internalLayout().BeginPtr()[n];
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reference
	basic_string<T, Safety>::at(size_type n)
	{
		if(NODECPP_UNLIKELY(n >= internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		return internalLayout().BeginPtr()[n];
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reference
	basic_string<T, Safety>::front()
	{
		if constexpr(is_safe == memory_safety::safe) {
			// We assert if the user references the trailing 0 char.
			if(NODECPP_UNLIKELY(internalLayout().GetSize() == 0))
				ThrowRangeException("basic_string::front -- empty string");
		}

		return *internalLayout().BeginPtr();
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reference
	basic_string<T, Safety>::front() const
	{
		if constexpr(is_safe == memory_safety::safe) {
			// We assert if the user references the trailing 0 char.
			if(NODECPP_UNLIKELY(internalLayout().GetSize() == 0))
				ThrowRangeException("basic_string::front -- empty string");
		}

		return *internalLayout().BeginPtr();
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reference
	basic_string<T, Safety>::back()
	{
		if constexpr(is_safe == memory_safety::safe) {
			// We assert if the user references the trailing 0 char.
			if(NODECPP_UNLIKELY(internalLayout().GetSize() == 0))
				ThrowRangeException("basic_string::back -- empty string");
		}

		return *(internalLayout().EndPtr() - 1);
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_reference
	basic_string<T, Safety>::back() const
	{
		if constexpr(is_safe == memory_safety::safe) {
			// We assert if the user references the trailing 0 char.
			if(NODECPP_UNLIKELY(internalLayout().GetSize() == 0))
				ThrowRangeException("basic_string::back -- empty string");
		}

		return *(internalLayout().EndPtr() - 1);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::operator+=(const this_type& x)
	{
		return append(x);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::operator+=(literal_type x)
	{
		return append(x);
	}

	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>& basic_string<T, Safety>::operator+=(const_pointer p)
	// {
	// 	return append(p);
	// }


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::operator+=(value_type c)
	{
		push_back(c);
		return *this;
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::append(const this_type& x)
	{
		return append_unsafe(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::append(const this_type& x, size_type position, size_type n)
	{
		const_pointer_pair p = toPtrPair(x, position, n);
	    return append_unsafe(p.first, p.second);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::append(literal_type x)
	{
		const T* p = x.c_str();
		return append_unsafe(p, p + CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::append_unsafe(const_pointer p, size_type n)
	{
		return append_unsafe(p, p + n);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::append_unsafe(const_pointer p)
	{
		return append_unsafe(p, p + CharStrlen(p));
	}


	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::append_convert_unsafe(const OtherCharType* pOther)
	{
		return append_convert_unsafe(pOther, (size_type)CharStrlen(pOther));
	}

	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::append_convert(const basic_string<OtherCharType>& x)
	{
		return append_convert_unsafe(x.c_str(), x.length());
	}


	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::append_convert_unsafe(const OtherCharType* pOther, size_type n)
	{
		// Question: What do we do in the case that we have an illegally encoded source string?
		// This can happen with UTF8 strings. Do we throw an exception or do we ignore the input?
		// One argument is that it's not a string class' job to handle the security aspects of a
		// program and the higher level application code should be verifying UTF8 string validity,
		// and thus we should do the friendly thing and ignore the invalid characters as opposed
		// to making the user of this function handle exceptions that are easily forgotten.

		const std::size_t         kBufferSize = 512;
		value_type           selfBuffer[kBufferSize];   // This assumes that value_type is one of char8_t, char16_t, char32_t, or wchar_t. Or more importantly, a type with a trivial constructor and destructor.
		pointer const    selfBufferEnd = selfBuffer + kBufferSize;
		const OtherCharType* pOtherEnd = pOther + n;

		while(pOther != pOtherEnd)
		{
			pointer pSelfBufferCurrent = selfBuffer;
			DecodePart(pOther, pOtherEnd, pSelfBufferCurrent, selfBufferEnd);   // Write pOther to pSelfBuffer, converting encoding as we go. We currently ignore the return value, as we don't yet have a plan for handling encoding errors.
			append_unsafe(selfBuffer, pSelfBufferCurrent);
		}

		return *this;
	}

	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::append_convert(basic_string_literal<OtherCharType> x)
	{
		const T* p = x.c_str();
		return append_convert_unsafe(p, CharStrlen(p));
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::append(size_type n, value_type c)
	{
		if (n > 0)
		{
			const size_type nSize = internalLayout().GetSize();
			const size_type nCapacity = capacity();

			if((nSize + n) > nCapacity)
				reserve(GetNewCapacity(nCapacity, (nSize + n)));

			pointer pNewEnd = CharStringUninitializedFillN(internalLayout().EndPtr(), n, c);
			*pNewEnd = 0;
			internalLayout().SetSize(nSize + n);
		}

		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::append_unsafe(const_pointer pBegin, const_pointer pEnd)
	{
		if(pBegin != pEnd)
		{
			const size_type nOldSize  = internalLayout().GetSize();
			const size_type n         = (size_type)(pEnd - pBegin);
			const size_type nCapacity = capacity();
			const size_type nNewSize = nOldSize + n;

			if(nNewSize > nCapacity)
			{
				const size_type nLength = GetNewCapacity(nCapacity, nNewSize);

				owning_heap_type pNewBegin = DoAllocate(nLength + 1);

				pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), internalLayout().EndPtr(), pNewBegin->begin());
				pNewEnd         = CharStringUninitializedCopy(pBegin,  pEnd,  pNewEnd);
			   *pNewEnd         = 0;

				// DeallocateSelf();
				internalLayout().SetNewHeap(std::move(pNewBegin));
//				internalLayout().SetHeapCapacity(nLength);
				internalLayout().SetSize(nNewSize);
			}
			else
			{
				pointer pNewEnd = CharStringUninitializedCopy(pBegin, pEnd, internalLayout().EndPtr());
				*pNewEnd = 0;
				internalLayout().SetSize(nNewSize);
			}
		}

		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::append(csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		const_pointer_pair p = checkAndGet(itBegin, itEnd);
		return append_unsafe(p.first, p.second);
	}
// 	template <typename T, memory_safety Safety>
// 	basic_string<T, Safety>& basic_string<T, Safety>::append_sprintf_va_list(const_pointer pFormat, va_list arguments)
// 	{
// 		// From unofficial C89 extension documentation:
// 		// The vsnprintf returns the number of characters written into the array,
// 		// not counting the terminating null character, or a negative value
// 		// if count or more characters are requested to be generated.
// 		// An error can occur while converting a value for output.

// 		// From the C99 standard:
// 		// The vsnprintf function returns the number of characters that would have
// 		// been written had n been sufficiently large, not counting the terminating
// 		// null character, or a negative value if an encoding error occurred.
// 		// Thus, the null-terminated output has been completely written if and only
// 		// if the returned value is nonnegative and less than n.

// 		// https://www.freebsd.org/cgi/man.cgi?query=vswprintf&sektion=3&manpath=freebsd-release-ports
// 		// https://www.freebsd.org/cgi/man.cgi?query=snprintf&manpath=SuSE+Linux/i386+11.3
// 		// Well its time to go on an adventure...
// 		// C99 vsnprintf states that a buffer size of zero returns the number of characters that would
// 		// be written to the buffer irrelevant of whether the buffer is a nullptr
// 		// But C99 vswprintf for wchar_t changes the behaviour of the return to instead say that it
// 		// "will fail if n or more wide characters were requested to be written", so
// 		// calling vswprintf with a buffer size of zero always returns -1
// 		// unless... you are MSVC where they deviate from the std and say if the buffer is NULL
// 		// and the size is zero it will return the number of characters written or if we are using
// 		// EAStdC which also does the sane behaviour.

// #if !EASTL_OPENSOURCE || defined(EA_PLATFORM_MICROSOFT)
// 		size_type nInitialSize = internalLayout().GetSize();
// 		int nReturnValue;

// 		#if EASTL_VA_COPY_ENABLED
// 			va_list argumentsSaved;
// 			va_copy(argumentsSaved, arguments);
// 		#endif

// 		nReturnValue = nodecpp::Vsnprintf(nullptr, 0, pFormat, arguments);

// 		if (nReturnValue > 0)
// 		{
// 			resize(nReturnValue + nInitialSize);

// 		#if EASTL_VA_COPY_ENABLED
// 			va_end(arguments);
// 			va_copy(arguments, argumentsSaved);
// 		#endif

// 			nReturnValue = nodecpp::Vsnprintf(internalLayout().BeginPtr() + nInitialSize, static_cast<size_t>(nReturnValue) + 1, pFormat, arguments);
// 		}

// 		if (nReturnValue >= 0)
// 			internalLayout().SetSize(nInitialSize + nReturnValue);

// 		#if EASTL_VA_COPY_ENABLED
// 			// va_end for arguments will be called by the caller.
// 			va_end(argumentsSaved);
// 		#endif

// #else
// 		size_type nInitialSize = internalLayout().GetSize();
// 		size_type nInitialRemainingCapacity = internalLayout().GetRemainingCapacity();
// 		int       nReturnValue;

// 		#if EASTL_VA_COPY_ENABLED
// 			va_list argumentsSaved;
// 			va_copy(argumentsSaved, arguments);
// 		#endif

// 		nReturnValue = nodecpp::Vsnprintf(internalLayout().EndPtr(), (size_t)nInitialRemainingCapacity + 1,
// 										pFormat, arguments);

// 		if(nReturnValue >= (int)(nInitialRemainingCapacity + 1))  // If there wasn't enough capacity...
// 		{
// 			// In this case we definitely have C99 Vsnprintf behaviour.
// 		#if EASTL_VA_COPY_ENABLED
// 			va_end(arguments);
// 			va_copy(arguments, argumentsSaved);
// 		#endif
// 			resize(nInitialSize + nReturnValue);
// 			nReturnValue = nodecpp::Vsnprintf(internalLayout().BeginPtr() + nInitialSize, (size_t)(nReturnValue + 1),
// 											pFormat, arguments);
// 		}
// 		else if(nReturnValue < 0) // If vsnprintf is non-C99-standard
// 		{
// 			// In this case we either have C89 extension behaviour or C99 behaviour.
// 			size_type n = std::max((size_type)(SSOLayout::SSO_CAPACITY - 1), (size_type)(nInitialSize * 2));

// 			for(; (nReturnValue < 0) && (n < 1000000); n *= 2)
// 			{
// 			#if EASTL_VA_COPY_ENABLED
// 				va_end(arguments);
// 				va_copy(arguments, argumentsSaved);
// 			#endif
// 				resize(n);

// 				const size_t nCapacity = (size_t)(n - nInitialSize);
// 				nReturnValue = nodecpp::Vsnprintf(internalLayout().BeginPtr() + nInitialSize, nCapacity + 1, pFormat, arguments);

// 				if(nReturnValue == (int)(unsigned)nCapacity)
// 				{
// 					resize(++n);
// 					nReturnValue = nodecpp::Vsnprintf(internalLayout().BeginPtr() + nInitialSize, nCapacity + 2, pFormat, arguments);
// 				}
// 			}
// 		}

// 		if(nReturnValue >= 0)
// 			internalLayout().SetSize(nInitialSize + nReturnValue);

// 		#if EASTL_VA_COPY_ENABLED
// 			// va_end for arguments will be called by the caller.
// 			va_end(argumentsSaved);
// 		#endif

// #endif // EASTL_OPENSOURCE

// 		return *this;
// 	}

// 	template <typename T, memory_safety Safety>
// 	basic_string<T, Safety>& basic_string<T, Safety>::append_sprintf(const_pointer pFormat, ...)
// 	{
// 		va_list arguments;
// 		va_start(arguments, pFormat);
// 		append_sprintf_va_list(pFormat, arguments);
// 		va_end(arguments);

// 		return *this;
// 	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::push_back(value_type c)
	{
		append((size_type)1, c);
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::pop_back()
	{
		if constexpr(is_safe == memory_safety::safe) {
			// We assert if the user references the trailing 0 char.
			if(NODECPP_UNLIKELY(internalLayout().GetSize() == 0))
				ThrowRangeException("basic_string::pop_back -- empty string");
		}

		internalLayout().EndPtr()[-1] = value_type(0);
		internalLayout().SetSize(internalLayout().GetSize() - 1);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign(const this_type& x)
	{
		// The C++11 Standard 21.4.6.3 p6 specifies that assign from this_type assigns contents only and not the allocator.
		return assign_unsafe(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign(const this_type& x, size_type position, size_type n)
	{
		const_pointer_pair p = toPtrPair(x, position, n);
		return assign_unsafe(p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign(literal_type x)
	{
		const T* p = x.c_str();
		return assign_unsafe(p, p + CharStrlen(p));
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign_unsafe(const_pointer p, size_type n)
	{
		return assign_unsafe(p, p + n);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign_unsafe(const_pointer p)
	{
		return assign_unsafe(p, p + CharStrlen(p));
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::assign(size_type n, value_type c)
	{
		if(n <= internalLayout().GetSize())
		{
			CharTypeAssignN(internalLayout().BeginPtr(), n, c);
			erase_unsafe(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
		}
		else
		{
			CharTypeAssignN(internalLayout().BeginPtr(), internalLayout().GetSize(), c);
			append(n - internalLayout().GetSize(), c);
		}
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::assign_unsafe(const_pointer pBegin, const_pointer pEnd)
	{
		const size_type n = (size_type)(pEnd - pBegin);
		if(n <= internalLayout().GetSize())
		{
			memmove(internalLayout().BeginPtr(), pBegin, (std::size_t)n * sizeof(value_type));
			erase_unsafe(internalLayout().BeginPtr() + n, internalLayout().EndPtr());
		}
		else
		{
			memmove(internalLayout().BeginPtr(), pBegin, (std::size_t)(internalLayout().GetSize()) * sizeof(value_type));
			append_unsafe(pBegin + internalLayout().GetSize(), pEnd);
		}
		return *this;
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::assign(csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		const_pointer_pair p = checkAndGet(itBegin, itEnd);
		return assign_unsafe(p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign(std::initializer_list<value_type> ilist)
	{
		return assign_unsafe(ilist.begin(), ilist.end());
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::assign(this_type&& x)
	{
		// if(get_allocator() == x.get_allocator())
		// {
			std::swap(internalLayout(), x.internalLayout());
		// }
		// else
		// 	assign(x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());

		return *this;
	}


	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::assign_convert_unsafe(const OtherCharType* p)
	{
		clear();
		append_convert_unsafe(p);
		return *this;
	}


	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::assign_convert_unsafe(const OtherCharType* p, size_type n)
	{
		clear();
		append_convert_unsafe(p, n);
		return *this;
	}

	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::assign_convert(const basic_string<OtherCharType>& x)
	{
		clear();
		append_convert_unsafe(x.data(), x.length());
		return *this;
	}

	template <typename T, memory_safety Safety>
	template <typename OtherCharType>
	basic_string<T, Safety>& basic_string<T, Safety>::assign_convert(basic_string_literal<OtherCharType> x)
	{
		clear();
		append_convert_unsafe(x.c_str());
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert(size_type position, const this_type& x)
	{
		if(NODECPP_UNLIKELY(position > internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		insert_unsafe(internalLayout().BeginPtr() + position, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert(size_type position, const this_type& x, size_type beg, size_type n)
	{
		if(NODECPP_UNLIKELY(position > internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		const_pointer_pair ot = toPtrPair(x, beg, n);

		insert_unsafe(internalLayout().BeginPtr() + position, ot.first, ot.second);
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert_unsafe(size_type position, const_pointer p, size_type n)
	{
		insert_unsafe(internalLayout().BeginPtr() + position, p, p + n);
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert_unsafe(size_type position, const_pointer p)
	{
		size_type nLength = (size_type)CharStrlen(p);

		insert_unsafe(internalLayout().BeginPtr() + position, p, p + nLength);
		return *this;
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert(size_type position, literal_type x)
	{
		if(NODECPP_UNLIKELY(position > internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		const_pointer_pair p = toPtrPair(x);
		insert_unsafe(internalLayout().BeginPtr() + position, p.first, p.second);
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::insert(size_type position, size_type n, value_type c)
	{
		if(NODECPP_UNLIKELY(position > internalLayout().GetSize()))
			ThrowRangeException("basic_string -- out of range");

		insert_unsafe(internalLayout().BeginPtr() + position, n, c);
		return *this;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::insert_unsafe(const_pointer p, value_type c)
	{
		if(p == const_pointer(internalLayout().EndPtr()))
		{
			push_back(c);
			return internalLayout().EndPtr() - 1;
		}
		return InsertInternal(p, c);
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::insert_unsafe(const_pointer p, size_type n, value_type c)
	{
		const difference_type nPosition = (p - internalLayout().BeginPtr()); // Save this because we might reallocate.

		if(n) // If there is anything to insert...
		{
			if(internalLayout().GetRemainingCapacity() >= n) // If we have enough capacity...
			{
				const size_type nElementsAfter = (size_type)(internalLayout().EndPtr() - p);

				if(nElementsAfter >= n) // If there's enough space for the new chars between the insert position and the end...
				{
					// Ensure we save the size before we do the copy, as we might overwrite the size field with the NULL
					// terminator in the edge case where we are inserting enough characters to equal our capacity
					const size_type nSavedSize = internalLayout().GetSize();
					CharStringUninitializedCopy((internalLayout().EndPtr() - n) + 1, internalLayout().EndPtr() + 1, internalLayout().EndPtr() + 1);
					internalLayout().SetSize(nSavedSize + n);
					memmove(const_cast<pointer>(p) + n, p, (std::size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
					CharTypeAssignN(const_cast<pointer>(p), n, c);
				}
				else
				{
					pointer pOldEnd = internalLayout().EndPtr();
					// #if EASTL_EXCEPTIONS_ENABLED
						const size_type nOldSize = internalLayout().GetSize();
					// #endif
					CharStringUninitializedFillN(internalLayout().EndPtr() + 1, n - nElementsAfter - 1, c);
					internalLayout().SetSize(internalLayout().GetSize() + (n - nElementsAfter));

					// #if EASTL_EXCEPTIONS_ENABLED
						try
						{
					// #endif
							// See comment in if block above
							const size_type nSavedSize = internalLayout().GetSize();
							CharStringUninitializedCopy(p, pOldEnd + 1, internalLayout().EndPtr());
							internalLayout().SetSize(nSavedSize + nElementsAfter);
					// #if EASTL_EXCEPTIONS_ENABLED
						}
						catch(...)
						{
							internalLayout().SetSize(nOldSize);
							throw;
						}
					// #endif

					CharTypeAssignN(const_cast<pointer>(p), nElementsAfter + 1, c);
				}
			}
			else
			{
				const size_type nOldSize = internalLayout().GetSize();
				const size_type nOldCap  = capacity();
				const size_type nLength  = GetNewCapacity(nOldCap, (nOldSize + n));

				owning_heap_type pNewBegin = DoAllocate(nLength + 1);

				pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin->begin());
				pNewEnd          = CharStringUninitializedFillN(pNewEnd, n, c);
				pNewEnd          = CharStringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
			   *pNewEnd          = 0;

				// DeallocateSelf();
				internalLayout().SetNewHeap(std::move(pNewBegin));
				// internalLayout().SetHeapCapacity(nLength);
				internalLayout().SetSize(nOldSize + n);
			}
		}

		return internalLayout().BeginPtr() + nPosition;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::insert_unsafe(const_pointer p, const_pointer pBegin, const_pointer pEnd)
	{
		const difference_type nPosition = (p - internalLayout().BeginPtr()); // Save this because we might reallocate.

		const size_type n = (size_type)(pEnd - pBegin);

		if(n)
		{
			const bool bCapacityIsSufficient = (internalLayout().GetRemainingCapacity() >= n);
			const bool bSourceIsFromSelf     = ((pEnd >= internalLayout().BeginPtr()) && (pBegin <= internalLayout().EndPtr()));

			// if(bSourceIsFromSelf && internalLayout().IsSSO())
			// {
			// 	// pBegin to pEnd will be <= this->GetSize(), so stackTemp will guaranteed be an SSO String
			// 	// If we are inserting ourself into ourself and we are SSO, then on the recursive call we can
			// 	// guarantee 0 or 1 allocation depending if we need to realloc
			// 	// We don't do this for Heap strings as then this path may do 1 or 2 allocations instead of
			// 	// only 1 allocation when we fall through to the last else case below
			// 	const this_type stackTemp(pBegin, pEnd/*, get_allocator()*/);
			// 	return insert(p, stackTemp.data(), stackTemp.data() + stackTemp.size());
			// }

			// If bSourceIsFromSelf is true, then we reallocate. This is because we are
			// inserting ourself into ourself and thus both the source and destination
			// be modified, making it rather tricky to attempt to do in place. The simplest
			// resolution is to reallocate. To consider: there may be a way to implement this
			// whereby we don't need to reallocate or can often avoid reallocating.
			if(bCapacityIsSufficient && !bSourceIsFromSelf)
			{
				const size_type nElementsAfter = (size_type)(internalLayout().EndPtr() - p);

				if(nElementsAfter >= n) // If there are enough characters between insert pos and end
				{
					// Ensure we save the size before we do the copy, as we might overwrite the size field with the NULL
					// terminator in the edge case where we are inserting enough characters to equal our capacity
					const size_type nSavedSize = internalLayout().GetSize();
					CharStringUninitializedCopy((internalLayout().EndPtr() - n) + 1, internalLayout().EndPtr() + 1, internalLayout().EndPtr() + 1);
					internalLayout().SetSize(nSavedSize + n);
					memmove(const_cast<pointer>(p) + n, p, (std::size_t)((nElementsAfter - n) + 1) * sizeof(value_type));
					memmove(const_cast<pointer>(p), pBegin, (std::size_t)(n) * sizeof(value_type));
				}
				else
				{
					pointer pOldEnd = internalLayout().EndPtr();
					// #if EASTL_EXCEPTIONS_ENABLED
						const size_type nOldSize = internalLayout().GetSize();
					// #endif
					const_pointer const pMid = pBegin + (nElementsAfter + 1);

					CharStringUninitializedCopy(pMid, pEnd, internalLayout().EndPtr() + 1);
					internalLayout().SetSize(internalLayout().GetSize() + (n - nElementsAfter));

					// #if EASTL_EXCEPTIONS_ENABLED
						try
						{
					// #endif
							// See comment in if block above
							const size_type nSavedSize = internalLayout().GetSize();
							CharStringUninitializedCopy(p, pOldEnd + 1, internalLayout().EndPtr());
							internalLayout().SetSize(nSavedSize + nElementsAfter);
					// #if EASTL_EXCEPTIONS_ENABLED
						}
						catch(...)
						{
							internalLayout().SetSize(nOldSize);
							throw;
						}
					// #endif

					CharStringUninitializedCopy(pBegin, pMid, const_cast<pointer>(p));
				}
			}
			else // Else we need to reallocate to implement this.
			{
				const size_type nOldSize = internalLayout().GetSize();
				const size_type nOldCap  = capacity();
				size_type nLength;

				if(bCapacityIsSufficient) // If bCapacityIsSufficient is true, then bSourceIsFromSelf must be true.
					nLength = nOldSize + n;
				else
					nLength = GetNewCapacity(nOldCap, (nOldSize + n));

				owning_heap_type pNewBegin = DoAllocate(nLength + 1);

				pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin->begin());
				pNewEnd         = CharStringUninitializedCopy(pBegin, pEnd, pNewEnd);
				pNewEnd         = CharStringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
			   *pNewEnd         = 0;

				// DeallocateSelf();
				internalLayout().SetNewHeap(std::move(pNewBegin));
				// internalLayout().SetHeapCapacity(nLength);
				internalLayout().SetSize(nOldSize + n);
			}
		}

		return internalLayout().BeginPtr() + nPosition;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::insert_unsafe(const_pointer p, std::initializer_list<value_type> ilist)
	{
		return insert_unsafe(p, ilist.begin(), ilist.end());
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::insert(csafe_it_arg it, value_type c)
	{
		const_pointer p = checkMineAndGet(it);
		pointer r = insert_unsafe(p, c);
		return iterator::make(GetSoftHeapPtr(), r);
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::insert(csafe_it_arg it, size_type n, value_type c)
	{
		const_pointer p = checkMineAndGet(it);
		pointer r = insert_unsafe(p, n, c);
		return iterator::make(GetSoftHeapPtr(), r);
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::insert(csafe_it_arg it, csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		const_pointer p = checkMineAndGet(it);
		const_pointer_pair p2 = checkAndGet(itBegin, itEnd);
		pointer r = insert_unsafe(p, p2.first, p2.second);
		return iterator::make(GetSoftHeapPtr(), r);
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::insert(csafe_it_arg it, std::initializer_list<value_type> ilist)
	{
		const_pointer p = checkMineAndGet(it);
		pointer r = insert_unsafe(p, ilist.begin(), ilist.end());
		return iterator::make(GetSoftHeapPtr(), r);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::erase(size_type position, size_type n)
	{
		const_pointer_pair p = toPtrPair(*this, position, n);
		erase_unsafe(p.first, p.second);

		return *this;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::erase_unsafe(const_pointer p)
	{
		memmove(const_cast<pointer>(p), p + 1, (std::size_t)(internalLayout().EndPtr() - p) * sizeof(value_type));
		internalLayout().SetSize(internalLayout().GetSize() - 1);
		return const_cast<pointer>(p);
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::erase_unsafe(const_pointer pBegin, const_pointer pEnd)
	{
		if(pBegin != pEnd)
		{
			memmove(const_cast<pointer>(pBegin), pEnd, (std::size_t)((internalLayout().EndPtr() - pEnd) + 1) * sizeof(value_type));
			const size_type n = (size_type)(pEnd - pBegin);
			internalLayout().SetSize(internalLayout().GetSize() - n);
		}
		return pointer(const_cast<pointer>(pBegin));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::erase(csafe_it_arg it)
	{
		const_pointer p = checkMineAndGet(it);
		pointer i = erase_unsafe(p);
		// size_t dst = std::distance(begin_unsafe(),  i);
		return iterator::make(GetSoftHeapPtr(), i);
	}
	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::iterator
	basic_string<T, Safety>::erase(csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		const_pointer_pair p = checkMineAndGet(itBegin, itEnd);
		pointer i = erase_unsafe(p.first, p.second);
		// difference_type dst = std::distance(begin_unsafe(),  i);
		return iterator::make(GetSoftHeapPtr(), i);
	}

	// template <typename T, memory_safety Safety>
	// inline typename basic_string<T, Safety>::reverse_iterator_unsafe
	// basic_string<T, Safety>::erase_unsafe(reverse_iterator_unsafe position)
	// {
	// 	return reverse_iterator_unsafe(erase_unsafe((++position).base()));
	// }


	// template <typename T, memory_safety Safety>
	// typename basic_string<T, Safety>::reverse_iterator_unsafe
	// basic_string<T, Safety>::erase_unsafe(reverse_iterator_unsafe first, reverse_iterator_unsafe last)
	// {
	// 	return reverse_iterator_unsafe(erase_unsafe((++last).base(), (++first).base()));
	// }

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::reverse_iterator
	basic_string<T, Safety>::erase(crsafe_it_arg position)
	{
		return reverse_iterator(erase((++position).base()));
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::reverse_iterator
	basic_string<T, Safety>::erase(crsafe_it_arg first, crsafe_it_arg last)
	{
		return reverse_iterator(erase((++last).base(), (++first).base()));
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace(size_type position, size_type n, const this_type& x)
	{
		const_pointer_pair m = toPtrPair(*this, position, n);

		return replace_unsafe(m.first, m.second, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2)
	{
		const_pointer_pair p1 = toPtrPair(*this, pos1, n1);
		const_pointer_pair p2 = toPtrPair(x, pos2, n2);

		return replace_unsafe(p1.first, p1.second, p2.first, p2.second);
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(size_type position, size_type n1, const_pointer p, size_type n2)
	{
		const size_type nLength = std::min(n1, internalLayout().GetSize() - position);

		return replace_unsafe(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p, p + n2);
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(size_type position, size_type n1, const_pointer p)
	{
		const size_type nLength = std::min(n1, internalLayout().GetSize() - position);

		return replace_unsafe(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p, p + CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace(size_type position, size_type n1, literal_type x)
	{
		const_pointer_pair t = toPtrPair(*this, position, n1);
		const_pointer_pair p = toPtrPair(x);

		return replace_unsafe(t.first, t.second, p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace(size_type position, size_type n1, size_type n2, value_type c)
	{
		const_pointer_pair p = toPtrPair(*this, position, n1);

		return replace_unsafe(p.first, p.second, n2, c);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_pointer pBegin, const_pointer pEnd, const this_type& x)
	{
		return replace_unsafe(pBegin, pEnd, x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_pointer pBegin, const_pointer pEnd, const_pointer p, size_type n)
	{
		return replace_unsafe(pBegin, pEnd, p, p + n);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_pointer pBegin, const_pointer pEnd, const_pointer p)
	{
		return replace_unsafe(pBegin, pEnd, p, p + CharStrlen(p));
	}

	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_iterator pBegin, const_iterator pEnd, literal_type x)
	// {
	// 	const T* p = x.c_str();
	// 	return replace_unsafe(pBegin, pEnd, p, p + CharStrlen(p));
	// }

	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_pointer pBegin, const_pointer pEnd, size_type n, value_type c)
	{
		const size_type nLength = static_cast<size_type>(pEnd - pBegin);

		if(nLength >= n)
		{
			CharTypeAssignN(const_cast<pointer>(pBegin), n, c);
			erase_unsafe(pBegin + n, pEnd);
		}
		else
		{
			CharTypeAssignN(const_cast<pointer>(pBegin), nLength, c);
			insert_unsafe(pEnd, n - nLength, c);
		}
		return *this;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety>& basic_string<T, Safety>::replace_unsafe(const_pointer pBegin1, const_pointer pEnd1, const_pointer pBegin2, const_pointer pEnd2)
	{
		const size_type nLength1 = (size_type)(pEnd1 - pBegin1);
		const size_type nLength2 = (size_type)(pEnd2 - pBegin2);

		if(nLength1 >= nLength2) // If we have a non-expanding operation...
		{
			if((pBegin2 > pEnd1) || (pEnd2 <= pBegin1))  // If we have a non-overlapping operation...
				memcpy(const_cast<pointer>(pBegin1), pBegin2, (std::size_t)(pEnd2 - pBegin2) * sizeof(value_type));
			else
				memmove(const_cast<pointer>(pBegin1), pBegin2, (std::size_t)(pEnd2 - pBegin2) * sizeof(value_type));
			erase_unsafe(pBegin1 + nLength2, pEnd1);
		}
		else // Else we are expanding.
		{
			if((pBegin2 > pEnd1) || (pEnd2 <= pBegin1)) // If we have a non-overlapping operation...
			{
				const_pointer const pMid2 = pBegin2 + nLength1;

				if((pEnd2 <= pBegin1) || (pBegin2 > pEnd1))
					memcpy(const_cast<pointer>(pBegin1), pBegin2, (std::size_t)(pMid2 - pBegin2) * sizeof(value_type));
				else
					memmove(const_cast<pointer>(pBegin1), pBegin2, (std::size_t)(pMid2 - pBegin2) * sizeof(value_type));
				insert_unsafe(pEnd1, pMid2, pEnd2);
			}
			else // else we have an overlapping operation.
			{
				// I can't think of any easy way of doing this without allocating temporary memory.
				const size_type nOldSize     = internalLayout().GetSize();
				const size_type nOldCap      = capacity();
				const size_type nNewCapacity = GetNewCapacity(nOldCap, (nOldSize + (nLength2 - nLength1)));

				owning_heap_type pNewBegin = DoAllocate(nNewCapacity + 1);

				pointer pNewEnd = CharStringUninitializedCopy(internalLayout().BeginPtr(), pBegin1, pNewBegin->begin());
				pNewEnd         = CharStringUninitializedCopy(pBegin2, pEnd2,   pNewEnd);
				pNewEnd         = CharStringUninitializedCopy(pEnd1,   internalLayout().EndPtr(),   pNewEnd);
			   *pNewEnd         = 0;

				// DeallocateSelf();
				internalLayout().SetNewHeap(std::move(pNewBegin));
//				internalLayout().SetHeapCapacity(nNewCapacity);
				internalLayout().SetSize(nOldSize + (nLength2 - nLength1));
			}
		}
		return *this;
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace(csafe_it_arg first, csafe_it_arg last, const this_type& x)
	{
		const_pointer_pair p = checkMineAndGet(first, last);

		return replace_unsafe(p.first, p.second, x);
	}

	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>& basic_string<T, Safety>::replace(csafe_it_arg first, csafe_it_arg last, const_pointer p, size_type n)
	// {
	// 	const_pointer_pair p = checkMineAndGet(first, last);
	// 	return replace_unsafe(p.first, p.second, p, n);
	// }

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace(csafe_it_arg first, csafe_it_arg last, literal_type x)
	{
		const_pointer_pair p = checkMineAndGet(first, last);
		return replace_unsafe(p.first, p.second, x);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace(csafe_it_arg first, csafe_it_arg last, size_type n, value_type c)
	{
		const_pointer_pair p = checkMineAndGet(first, last);
		return replace_unsafe(p.first, p.second, n, c);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety>& basic_string<T, Safety>::replace(csafe_it_arg first, csafe_it_arg last, csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		const_pointer_pair p = checkMineAndGet(first, last);
		const_pointer_pair it = checkMineAndGet(itBegin, itEnd);
		return replace_unsafe(p.first, p.second, it.first, it.second);
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::copy_unsafe(pointer p, size_type n, size_type position) const
	{
		// C++ std says the effects of this function are as if calling char_traits::copy()
		// thus the 'p' must not overlap *this string, so we can use memcpy
		const size_type nLength = std::min(n, internalLayout().GetSize() - position);
		CharStringUninitializedCopy(internalLayout().BeginPtr() + position, internalLayout().BeginPtr() + position + nLength, p);
		return nLength;
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::swap(this_type& x) noexcept
	{
		// if(get_allocator() == x.get_allocator() || (internalLayout().IsSSO() && x.internalLayout().IsSSO())) // If allocators are equivalent...
		// {
			// We leave mAllocator as-is.
			std::swap(internalLayout(), x.internalLayout());
		// }
		// else // else swap the contents.
		// {
		// 	const this_type temp(*this); // Can't call eastl::swap because that would
		// 	*this = x;                   // itself call this member swap function.
		// 	x     = temp;
		// }
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find(const this_type& x, size_type position) const noexcept
	{
		return find_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_unsafe(const_pointer p, size_type position) const
	{
		return find_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return find_unsafe(p, position, CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_unsafe(const_pointer p, size_type position, size_type n) const
	{
		if(NODECPP_LIKELY((position < internalLayout().GetSize())))
		{
			const_pointer const pBegin = internalLayout().BeginPtr() + position;
			const const_pointer pResult   = CharTypeStringSearch(pBegin, internalLayout().EndPtr(), p, p + n);

			if(pResult != internalLayout().EndPtr())
				return std::distance(internalLayout().BeginPtr(), pResult);
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find(value_type c, size_type position) const noexcept
	{
		if(NODECPP_LIKELY(position < internalLayout().GetSize()))// If the position is valid...
		{
			const const_pointer pResult = std::find(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), c);

			if(pResult != internalLayout().EndPtr())
				return (size_type)(pResult - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::rfind(const this_type& x, size_type position) const noexcept
	{
		return rfind_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::rfind_unsafe(const_pointer p, size_type position) const
	{
		return rfind_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::rfind(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return rfind_unsafe(p, position, CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::rfind_unsafe(const_pointer p, size_type position, size_type n) const
	{
		// Disabled because it's not clear what values are valid for position.
		// It is documented that npos is a valid value, though. We return npos and
		// don't crash if postion is any invalid value.
		//#if EASTL_ASSERT_ENABLED
		//    if(NODECPP_UNLIKELY((position != npos) && (position > (size_type)(mpEnd - mpBegin))))
		//        EASTL_FAIL_MSG("basic_string::rfind -- invalid position");
		//#endif

		// Note that a search for a zero length string starting at position = end() returns end() and not npos.
		// Note by Paul Pedriana: I am not sure how this should behave in the case of n == 0 and position > size.
		// The standard seems to suggest that rfind doesn't act exactly the same as find in that input position
		// can be > size and the return value can still be other than npos. Thus, if n == 0 then you can
		// never return npos, unlike the case with find.
		const size_type nLength = internalLayout().GetSize();

		if(NODECPP_LIKELY(n <= nLength))
		{
			if(NODECPP_LIKELY(n))
			{
				const const_pointer pEnd    = internalLayout().BeginPtr() + std::min(nLength - n, position) + n;
				const const_pointer pResult = CharTypeStringRSearch(internalLayout().BeginPtr(), pEnd, p, p + n);

				if(pResult != pEnd)
					return (size_type)(pResult - internalLayout().BeginPtr());
			}
			else
				return std::min(nLength, position);
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::rfind(value_type c, size_type position) const noexcept
	{
		// If n is zero or position is >= size, we return npos.
		const size_type nLength = internalLayout().GetSize();

		if(NODECPP_LIKELY(nLength))
		{
			const_pointer const pEnd    = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
			const_pointer const pResult = CharTypeStringRFind(pEnd, internalLayout().BeginPtr(), c);

			if(pResult != internalLayout().BeginPtr())
				return (size_type)((pResult - 1) - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_of(const this_type& x, size_type position) const noexcept
	{
		return find_first_of_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_of_unsafe(const_pointer p, size_type position) const
	{
		return find_first_of_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_of(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return find_first_of_unsafe(p, position, CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_of_unsafe(const_pointer p, size_type position, size_type n) const
	{
		// If position is >= size, we return npos.
		if(NODECPP_LIKELY((position < internalLayout().GetSize())))
		{
			const_pointer const pBegin = internalLayout().BeginPtr() + position;
			const const_pointer pResult   = CharTypeStringFindFirstOf(pBegin, internalLayout().EndPtr(), p, p + n);

			if(pResult != internalLayout().EndPtr())
				return (size_type)(pResult - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_of(value_type c, size_type position) const noexcept
	{
		return find(c, position);
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_of(const this_type& x, size_type position) const noexcept
	{
		return find_last_of_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_of_unsafe(const_pointer p, size_type position) const
	{
		return find_last_of_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_of(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return find_last_of_unsafe(p, position, CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_of_unsafe(const_pointer p, size_type position, size_type n) const
	{
		// If n is zero or position is >= size, we return npos.
		const size_type nLength = internalLayout().GetSize();

		if(NODECPP_LIKELY(nLength))
		{
			const_pointer const pEnd    = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
			const_pointer const pResult = CharTypeStringRFindFirstOf(pEnd, internalLayout().BeginPtr(), p, p + n);

			if(pResult != internalLayout().BeginPtr())
				return (size_type)((pResult - 1) - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_of(value_type c, size_type position) const noexcept
	{
		return rfind(c, position);
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_not_of(const this_type& x, size_type position) const noexcept
	{
		return find_first_not_of_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_not_of_unsafe(const_pointer p, size_type position) const
	{
		return find_first_not_of_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_not_of(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return find_first_not_of_unsafe(p, position, CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_not_of_unsafe(const_pointer p, size_type position, size_type n) const
	{
		if(NODECPP_LIKELY(position <= internalLayout().GetSize()))
		{
			const const_pointer pResult =
			    CharTypeStringFindFirstNotOf(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), p, p + n);

			if(pResult != internalLayout().EndPtr())
				return (size_type)(pResult - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_first_not_of(value_type c, size_type position) const noexcept
	{
		if(NODECPP_LIKELY(position <= internalLayout().GetSize()))
		{
			// Todo: Possibly make a specialized version of CharTypeStringFindFirstNotOf(pBegin, pEnd, c).
			const const_pointer pResult =
			    CharTypeStringFindFirstNotOf(internalLayout().BeginPtr() + position, internalLayout().EndPtr(), &c, &c + 1);

			if(pResult != internalLayout().EndPtr())
				return (size_type)(pResult - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_not_of(const this_type& x, size_type position) const noexcept
	{
		return find_last_not_of_unsafe(x.internalLayout().BeginPtr(), position, x.internalLayout().GetSize());
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_not_of_unsafe(const_pointer p, size_type position) const
	{
		return find_last_not_of_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_not_of(literal_type x, size_type position) const
	{
		const T* p = x.c_str();
		return find_last_not_of_unsafe(p, position, (size_type)CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_not_of_unsafe(const_pointer p, size_type position, size_type n) const
	{
		const size_type nLength = internalLayout().GetSize();

		if(NODECPP_LIKELY(nLength))
		{
			const_pointer const pEnd    = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
			const_pointer const pResult = CharTypeStringRFindFirstNotOf(pEnd, internalLayout().BeginPtr(), p, p + n);

			if(pResult != internalLayout().BeginPtr())
				return (size_type)((pResult - 1) - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::find_last_not_of(value_type c, size_type position) const noexcept
	{
		const size_type nLength = internalLayout().GetSize();

		if(NODECPP_LIKELY(nLength))
		{
			// Todo: Possibly make a specialized version of CharTypeStringRFindFirstNotOf(pBegin, pEnd, c).
			const_pointer const pEnd    = internalLayout().BeginPtr() + std::min(nLength - 1, position) + 1;
			const_pointer const pResult = CharTypeStringRFindFirstNotOf(pEnd, internalLayout().BeginPtr(), &c, &c + 1);

			if(pResult != internalLayout().BeginPtr())
				return (size_type)((pResult - 1) - internalLayout().BeginPtr());
		}
		return npos;
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety> basic_string<T, Safety>::substr(size_type position, size_type n) const
	{
		return basic_string(*this, position, n);
	}


	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare(const this_type& x) const noexcept
	{
		return compare_unsafe(internalLayout().BeginPtr(), internalLayout().EndPtr(), x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare(size_type pos1, size_type n1, const this_type& x) const
	{
		const_pointer_pair p = toPtrPair(*this, pos1, n1);

		return compare_unsafe(p.first, p.second,
			x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	}


	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2) const
	{
		const_pointer_pair p1 = toPtrPair(*this, pos1, n1);
		const_pointer_pair p2 = toPtrPair(x, pos2, n2);

		return compare_unsafe(p1.first, p1.second, p2.first, p2.second);
	}


	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare_unsafe(const_pointer p) const
	{
		return compare_unsafe(internalLayout().BeginPtr(), internalLayout().EndPtr(), p, p + CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare(literal_type x) const
	{
		const_pointer_pair p = toPtrPair(x);
		return compare_unsafe(internalLayout().BeginPtr(), internalLayout().EndPtr(), p.first, p.second);
	}

	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare_unsafe(size_type pos1, size_type n1, const_pointer p) const
	{
		return compare_unsafe(internalLayout().BeginPtr() + pos1,
					   internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
					   p,
					   p + CharStrlen(p));
	}

	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare(size_type pos1, size_type n1, literal_type x) const
	{
		const_pointer_pair p1 = toPtrPair(*this, pos1, n1);
		const_pointer_pair p2 = toPtrPair(x);

		return compare_unsafe(p1.first, p1.second, p2.first, p2.second);
	}

	template <typename T, memory_safety Safety>
	inline int basic_string<T, Safety>::compare_unsafe(size_type pos1, size_type n1, const_pointer p, size_type n2) const
	{
		return compare_unsafe(internalLayout().BeginPtr() + pos1,
					   internalLayout().BeginPtr() + pos1 + std::min(n1, internalLayout().GetSize() - pos1),
					   p,
					   p + n2);
	}


	// make_lower
	// This is a very simple ASCII-only case conversion function
	// Anything more complicated should use a more powerful separate library.
	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::make_lower()
	{
		for(pointer p = internalLayout().BeginPtr(); p < internalLayout().EndPtr(); ++p)
			*p = (value_type)CharToLower(*p);
	}


	// make_upper
	// This is a very simple ASCII-only case conversion function
	// Anything more complicated should use a more powerful separate library.
	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::make_upper()
	{
		for(pointer p = internalLayout().BeginPtr(); p < internalLayout().EndPtr(); ++p)
			*p = (value_type)CharToUpper(*p);
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::ltrim()
	{
		const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
		erase(0, find_first_not_of_unsafe(array));
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::rtrim()
	{
		const value_type array[] = { ' ', '\t', 0 }; // This is a pretty simplistic view of whitespace.
		erase(find_last_not_of_unsafe(array) + 1);
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::trim()
	{
		ltrim();
		rtrim();
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::ltrim_unsafe(const_pointer p)
	{
		erase(0, find_first_not_of_unsafe(p));
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::ltrim(literal_type x)
	{
		erase(0, find_first_not_of(x));
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::rtrim_unsafe(const_pointer p)
	{
		erase(find_last_not_of_unsafe(p) + 1);
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::rtrim(literal_type x)
	{
		erase(find_last_not_of(x) + 1);
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::trim_unsafe(const_pointer p)
	{
		ltrim_unsafe(p);
		rtrim_unsafe(p);
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::trim(literal_type x)
	{
		ltrim(x);
		rtrim(x);
	}

	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety> basic_string<T, Safety>::left(size_type n) const
	{
		const size_type nLength = length();
		if(n < nLength)
			return substr(0, n);
		// C++ std says that substr must return default constructed allocated, but we do not.
		// Instead it is much more practical to provide the copy of the current allocator
		return basic_string(*this/*, get_allocator()*/);
	}


	template <typename T, memory_safety Safety>
	inline basic_string<T, Safety> basic_string<T, Safety>::right(size_type n) const
	{
		const size_type nLength = length();
		if(n < nLength)
			return substr(nLength - n, n);
		// C++ std says that substr must return default constructed allocated, but we do not.
		// Instead it is much more practical to provide the copy of the current allocator
		return basic_string(*this/*, get_allocator()*/);
	}


	// template <typename T, memory_safety Safety>
	// inline basic_string<T, Safety>& basic_string<T, Safety>::sprintf(const_pointer pFormat, ...)
	// {
	// 	va_list arguments;
	// 	va_start(arguments, pFormat);
	// 	internalLayout().SetSize(0); // Fast truncate to zero length.
	// 	append_sprintf_va_list(pFormat, arguments);
	// 	va_end(arguments);

	// 	return *this;
	// }


	// template <typename T, memory_safety Safety>
	// basic_string<T, Safety>& basic_string<T, Safety>::sprintf_va_list(const_pointer pFormat, va_list arguments)
	// {
	// 	internalLayout().SetSize(0); // Fast truncate to zero length.

	// 	return append_sprintf_va_list(pFormat, arguments);
	// }


	/* static */
	template <typename T, memory_safety Safety>
	int basic_string<T, Safety>::compare_unsafe(const_pointer pBegin1, const_pointer pEnd1,
											const_pointer pBegin2, const_pointer pEnd2)
	{
		const difference_type n1   = pEnd1 - pBegin1;
		const difference_type n2   = pEnd2 - pBegin2;
		const difference_type nMin = std::min(n1, n2);
		const int       cmp  = Compare(pBegin1, pBegin2, static_cast<std::size_t>(nMin));

		return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
	}

	/* static */
	template <typename T, memory_safety Safety>
	int basic_string<T, Safety>::compare(csafe_it_arg itBegin1, csafe_it_arg itEnd1,
											csafe_it_arg itBegin2, csafe_it_arg itEnd2)
	{
		const_pointer_pair p1 = checkAndGet(itBegin1, itEnd1);
		const_pointer_pair p2 = checkAndGet(itBegin2, itEnd2);

		return compare_unsafe(p1.first, p1.second, p2.first, p2.second);
	}


	// template <typename T, memory_safety Safety>
	// int basic_string<T, Safety>::comparei(const_pointer pBegin1, const_pointer pEnd1,
	// 										 const_pointer pBegin2, const_pointer pEnd2)
	// {
	// 	const difference_type n1   = pEnd1 - pBegin1;
	// 	const difference_type n2   = pEnd2 - pBegin2;
	// 	const difference_type nMin = std::min(n1, n2);
	// 	const int       cmp  = CompareI(pBegin1, pBegin2, (size_t)nMin);

	// 	return (cmp != 0 ? cmp : (n1 < n2 ? -1 : (n1 > n2 ? 1 : 0)));
	// }


	// template <typename T, memory_safety Safety>
	// inline int basic_string<T, Safety>::comparei(const this_type& x) const noexcept
	// {
	// 	return comparei(internalLayout().BeginPtr(), internalLayout().EndPtr(), x.internalLayout().BeginPtr(), x.internalLayout().EndPtr());
	// }


	// template <typename T, memory_safety Safety>
	// inline int basic_string<T, Safety>::comparei(const_pointer p) const
	// {
	// 	return comparei(internalLayout().BeginPtr(), internalLayout().EndPtr(), p, p + CharStrlen(p));
	// }


	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::pointer
	basic_string<T, Safety>::InsertInternal(const_pointer p, value_type c)
	{
		pointer pNewPosition = const_cast<pointer>(p);

		if((internalLayout().EndPtr() + 1) <= internalLayout().CapacityPtr())
		{
			const size_type nSavedSize = internalLayout().GetSize();
			memmove(const_cast<pointer>(p) + 1, p, (std::size_t)(internalLayout().EndPtr() - p) * sizeof(value_type));
			*(internalLayout().EndPtr() + 1) = 0;
			*pNewPosition = c;
			internalLayout().SetSize(nSavedSize + 1);
		}
		else
		{
			const size_type nOldSize = internalLayout().GetSize();
			const size_type nOldCap  = capacity();
			const size_type nLength = GetNewCapacity(nOldCap, nOldCap + 1);

			owning_heap_type pNewBegin = DoAllocate(nLength + 1);

			pNewPosition = CharStringUninitializedCopy(internalLayout().BeginPtr(), p, pNewBegin->begin());
		   *pNewPosition = c;

			pointer pNewEnd = pNewPosition + 1;
			pNewEnd          = CharStringUninitializedCopy(p, internalLayout().EndPtr(), pNewEnd);
		   *pNewEnd          = 0;

			// DeallocateSelf();
			internalLayout().SetNewHeap(std::move(pNewBegin));
//			internalLayout().SetHeapCapacity(nLength);
			internalLayout().SetSize(nOldSize + 1);
		}
		return pNewPosition;
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::SizeInitialize(size_type n, value_type c)
	{
		AllocateSelf(n);

		CharStringUninitializedFillN(internalLayout().BeginPtr(), n, c);
		internalLayout().SetSize(n);
	   *internalLayout().EndPtr() = 0;
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::RangeInitialize(const_pointer pBegin, const_pointer pEnd)
	{
		const size_type n = (size_type)(pEnd - pBegin);

		AllocateSelf(n);

		CharStringUninitializedCopy(pBegin, pEnd, internalLayout().BeginPtr());
		internalLayout().SetSize(n);
	   *internalLayout().EndPtr() = 0;
	}


	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::RangeInitialize(const_pointer pBegin)
	{
		RangeInitialize(pBegin, pBegin + CharStrlen(pBegin));
	}


	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::owning_heap_type
	basic_string<T, Safety>::DoAllocate(size_type n)
	{
		if(NODECPP_UNLIKELY(n > max_size()))
			ThrowMaxSizeException("basic_string -- size too big");

		return detail::make_owning_array_of<T, Safety>(n);
	}


	// template <typename T, memory_safety Safety>
	// inline void basic_string<T, Safety>::DoFree(pointer p, size_type n)
	// {
	// 	if(p)
	// 		safe_memory::lib_helpers::EASTLFree(/*get_allocator(), */p, n * sizeof(value_type));
	// }


	/* static */
	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::size_type
	basic_string<T, Safety>::GetNewCapacity(size_type currentCapacity, size_type minimumRequiredCapacity)
	{
		if(NODECPP_UNLIKELY(minimumRequiredCapacity > kMaxSize))
			ThrowLengthException("basic_string::GetNewCapacity -- minimumRequiredCapacity > kMaxSize");

		const size_type nNewCapacity = std::max(minimumRequiredCapacity, currentCapacity * 2);

		return nNewCapacity;
	}

	template <typename T, memory_safety Safety>
	inline void basic_string<T, Safety>::AllocateSelf()
	{
		AllocateSelf(16);//TODO
//		internalLayout().Reset();
//		internalLayout().SetSSOSize(0);
	}


	template <typename T, memory_safety Safety>
	void basic_string<T, Safety>::AllocateSelf(size_type n)
	{
		if(NODECPP_UNLIKELY(n > max_size()))
			ThrowLengthException("basic_string::AllocateSelf -- n > max_size()");

		// if(n > SSOLayout::SSO_CAPACITY)
		// {
			owning_heap_type pBegin = DoAllocate(n + 1);
			internalLayout().SetNewHeap(std::move(pBegin));
//			internalLayout().SetHeapCapacity(n);
			internalLayout().SetSize(0);
			*internalLayout().BeginPtr() = value_type(0);			
		// }
		// else
		// 	AllocateSelf();
	}


	// template <typename T, memory_safety Safety>
	// inline void basic_string<T, Safety>::DeallocateSelf()
	// {
	// 	internalLayout().DoFree();
	// 	{
	// 		DoFree(internalLayout().BeginPtr(), internalLayout().GetHeapCapacity() + 1);
	// 	}
	// }

	/* static */
	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer_pair
	basic_string<T, Safety>::checkAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd)
	{
		if(NODECPP_LIKELY(itBegin <= itEnd)) {
			const_pointer b = itBegin.get_raw_ptr();
			const_pointer e = itEnd.get_raw_ptr();

			return const_pointer_pair(b, e);
		}

		ThrowInvalidArgumentException("basic_string -- invalid argument");
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer basic_string<T, Safety>::checkMineAndGet(csafe_it_arg it) const
	{
		if(NODECPP_LIKELY(it <= end())) {
			return it.get_raw_ptr();
		}

		ThrowInvalidArgumentException("basic_string -- invalid argument");
	}

	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer_pair
	basic_string<T, Safety>::checkMineAndGet(csafe_it_arg itBegin, csafe_it_arg itEnd) const
	{
		if(NODECPP_LIKELY(itBegin <= itEnd && itEnd <= end())) {
			const_pointer b = itBegin.get_raw_ptr();
			const_pointer e = itEnd.get_raw_ptr();

			return const_pointer_pair(b, e);
		}

		ThrowInvalidArgumentException("basic_string -- invalid argument");
	}

	/* static */
	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer_pair
	basic_string<T, Safety>::toPtrPair(literal_type lit)
	{
		//lit non-null warrantied

		const_pointer p = lit.c_str();
		return const_pointer_pair(p, p + CharStrlen(p));
	}

	/* static */
	template <typename T, memory_safety Safety>
	inline typename basic_string<T, Safety>::const_pointer_pair
	basic_string<T, Safety>::toPtrPair(const typename basic_string<T, Safety>::this_type& str, size_type pos, size_type n)
	{
		if(NODECPP_LIKELY(pos <= str.internalLayout().GetSize())) {
			const_pointer b = str.internalLayout().BeginPtr() + pos;

			size_type sz = std::min(n, str.internalLayout().GetSize() - pos);
			return const_pointer_pair(b, b + sz);
		}

		ThrowRangeException("basic_string -- out of range");
	}

	// CharTypeStringFindEnd
	// Specialized char version of STL find() from back function.
	// Not the same as RFind because search range is specified as forward iterators.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringFindEnd(const_pointer pBegin, const_pointer pEnd, value_type c)
	{
		const_pointer pTemp = pEnd;
		while(--pTemp >= pBegin)
		{
			if(*pTemp == c)
				return pTemp;
		}

		return pEnd;
	}


	// CharTypeStringRFind
	// Specialized value_type version of STL find() function in reverse.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringRFind(const_pointer pRBegin, const_pointer pREnd, const value_type c)
	{
		while(pRBegin > pREnd)
		{
			if(*(pRBegin - 1) == c)
				return pRBegin;
			--pRBegin;
		}
		return pREnd;
	}


	// CharTypeStringSearch
	// Specialized value_type version of STL search() function.
	// Purpose: find p2 within p1. Return p1End if not found or if either string is zero length.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringSearch(const_pointer p1Begin, const_pointer p1End,
													 const_pointer p2Begin, const_pointer p2End)
	{
		// Test for zero length strings, in which case we have a match or a failure,
		// but the return value is the same either way.
		if((p1Begin == p1End) || (p2Begin == p2End))
			return p1Begin;

		// Test for a pattern of length 1.
		if((p2Begin + 1) == p2End)
			return std::find(p1Begin, p1End, *p2Begin);

		// General case.
		const_pointer pTemp;
		const_pointer pTemp1 = (p2Begin + 1);
		const_pointer pCurrent = p1Begin;

		while(p1Begin != p1End)
		{
			p1Begin = std::find(p1Begin, p1End, *p2Begin);
			if(p1Begin == p1End)
				return p1End;

			pTemp = pTemp1;
			pCurrent = p1Begin;
			if(++pCurrent == p1End)
				return p1End;

			while(*pCurrent == *pTemp)
			{
				if(++pTemp == p2End)
					return p1Begin;
				if(++pCurrent == p1End)
					return p1End;
			}

			++p1Begin;
		}

		return p1Begin;
	}


	// CharTypeStringRSearch
	// Specialized value_type version of STL find_end() function (which really is a reverse search function).
	// Purpose: find last instance of p2 within p1. Return p1End if not found or if either string is zero length.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringRSearch(const_pointer p1Begin, const_pointer p1End,
													  const_pointer p2Begin, const_pointer p2End)
	{
		// Test for zero length strings, in which case we have a match or a failure,
		// but the return value is the same either way.
		if((p1Begin == p1End) || (p2Begin == p2End))
			return p1Begin;

		// Test for a pattern of length 1.
		if((p2Begin + 1) == p2End)
			return CharTypeStringFindEnd(p1Begin, p1End, *p2Begin);

		// Test for search string length being longer than string length.
		if((p2End - p2Begin) > (p1End - p1Begin))
			return p1End;

		// General case.
		const_pointer pSearchEnd = (p1End - (p2End - p2Begin) + 1);
		const_pointer pCurrent1;
		const_pointer pCurrent2;

		while(pSearchEnd != p1Begin)
		{
			// Search for the last occurrence of *p2Begin.
			pCurrent1 = CharTypeStringFindEnd(p1Begin, pSearchEnd, *p2Begin);
			if(pCurrent1 == pSearchEnd) // If the first char of p2 wasn't found,
				return p1End;           // then we immediately have failure.

			// In this case, *pTemp == *p2Begin. So compare the rest.
			pCurrent2 = p2Begin;
			while(*pCurrent1++ == *pCurrent2++)
			{
				if(pCurrent2 == p2End)
					return (pCurrent1 - (p2End - p2Begin));
			}

			// A smarter algorithm might know to subtract more than just one,
			// but in most cases it won't make much difference anyway.
			--pSearchEnd;
		}

		return p1End;
	}


	// CharTypeStringFindFirstOf
	// Specialized value_type version of STL find_first_of() function.
	// This function is much like the C runtime strtok function, except the strings aren't null-terminated.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringFindFirstOf(const_pointer p1Begin, const_pointer p1End,
														  const_pointer p2Begin, const_pointer p2End)
	{
		for( ; p1Begin != p1End; ++p1Begin)
		{
			for(const_pointer pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if(*p1Begin == *pTemp)
					return p1Begin;
			}
		}
		return p1End;
	}


	// CharTypeStringRFindFirstOf
	// Specialized value_type version of STL find_first_of() function in reverse.
	// This function is much like the C runtime strtok function, except the strings aren't null-terminated.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringRFindFirstOf(const_pointer p1RBegin, const_pointer p1REnd,
														   const_pointer p2Begin,  const_pointer p2End)
	{
		for( ; p1RBegin != p1REnd; --p1RBegin)
		{
			for(const_pointer pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if(*(p1RBegin - 1) == *pTemp)
					return p1RBegin;
			}
		}
		return p1REnd;
	}



	// CharTypeStringFindFirstNotOf
	// Specialized value_type version of STL find_first_not_of() function.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringFindFirstNotOf(const_pointer p1Begin, const_pointer p1End,
															 const_pointer p2Begin, const_pointer p2End)
	{
		for( ; p1Begin != p1End; ++p1Begin)
		{
			const_pointer pTemp;
			for(pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if(*p1Begin == *pTemp)
					break;
			}
			if(pTemp == p2End)
				return p1Begin;
		}
		return p1End;
	}


	// CharTypeStringRFindFirstNotOf
	// Specialized value_type version of STL find_first_not_of() function in reverse.
	template <typename T, memory_safety Safety>
	typename basic_string<T, Safety>::const_pointer
	basic_string<T, Safety>::CharTypeStringRFindFirstNotOf(const_pointer p1RBegin, const_pointer p1REnd,
															  const_pointer p2Begin,  const_pointer p2End)
	{
		for( ; p1RBegin != p1REnd; --p1RBegin)
		{
			const_pointer pTemp;
			for(pTemp = p2Begin; pTemp != p2End; ++pTemp)
			{
				if(*(p1RBegin-1) == *pTemp)
					break;
			}
			if(pTemp == p2End)
				return p1RBegin;
		}
		return p1REnd;
	}




	// iterator operators
	// template <typename T, memory_safety Safety>
	// inline bool operator==(const typename basic_string<T, Safety>::reverse_iterator_unsafe& r1,
	// 					   const typename basic_string<T, Safety>::reverse_iterator_unsafe& r2)
	// {
	// 	return r1.mpCurrent == r2.mpCurrent;
	// }


	// template <typename T, memory_safety Safety>
	// inline bool operator!=(const typename basic_string<T, Safety>::reverse_iterator_unsafe& r1,
	// 					   const typename basic_string<T, Safety>::reverse_iterator_unsafe& r2)
	// {
	// 	return r1.mpCurrent != r2.mpCurrent;
	// }


	// Operator +
	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		typedef typename basic_string<T, Safety>::CtorReserve CtorReserve;
		CtorReserve cDNI; // GCC 2.x forces us to declare a named temporary like this.
		basic_string<T, Safety> result(cDNI, a.size() + b.size()/*, const_cast<basic_string<T, Safety>&>(a).get_allocator()*/); // Note that we choose to assign a's allocator.
		result.append(a);
		result.append(b);
		return result;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		typedef typename basic_string<T, Safety>::CtorReserve CtorReserve;
		CtorReserve cDNI; // GCC 2.x forces us to declare a named temporary like this.
		const typename basic_string<T, Safety>::size_type n = CharStrlen(l.c_str());
		basic_string<T, Safety> result(cDNI, n + b.size()/*, const_cast<basic_string<T, Safety>&>(b).get_allocator()*/);
		result.append(l.c_str(), l.c_str() + n);
		result.append(b);
		return result;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(typename basic_string<T, Safety>::value_type c, const basic_string<T, Safety>& b)
	{
		typedef typename basic_string<T, Safety>::CtorReserve CtorReserve;
		CtorReserve cDNI; // GCC 2.x forces us to declare a named temporary like this.
		basic_string<T, Safety> result(cDNI, 1 + b.size()/*, const_cast<basic_string<T, Safety>&>(b).get_allocator()*/);
		result.push_back(c);
		result.append(b);
		return result;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		typedef typename basic_string<T, Safety>::CtorReserve CtorReserve;
		CtorReserve cDNI; // GCC 2.x forces us to declare a named temporary like this.
		const typename basic_string<T, Safety>::size_type n = CharStrlen(l.c_str());
		basic_string<T, Safety> result(cDNI, a.size() + n/*, const_cast<basic_string<T, Safety>&>(a).get_allocator()*/);
		result.append(a);
		result.append(l.c_str(), l.c_str() + n);
		return result;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::value_type c)
	{
		typedef typename basic_string<T, Safety>::CtorReserve CtorReserve;
		CtorReserve cDNI; // GCC 2.x forces us to declare a named temporary like this.
		basic_string<T, Safety> result(cDNI, a.size() + 1/*, const_cast<basic_string<T, Safety>&>(a).get_allocator()*/);
		result.append(a);
		result.push_back(c);
		return result;
	}


	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(basic_string<T, Safety>&& a, basic_string<T, Safety>&& b)
	{
		a.append(b); // Using an rvalue by name results in it becoming an lvalue.
		return std::move(a);
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(basic_string<T, Safety>&& a, const basic_string<T, Safety>& b)
	{
		a.append(b);
		return std::move(a);
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(typename basic_string<T, Safety>::literal_type l, basic_string<T, Safety>&& b)
	{
		b.insert(0, l);
		return std::move(b);
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(basic_string<T, Safety>&& a, typename basic_string<T, Safety>::literal_type l)
	{
		a.append(l);
		return std::move(a);
	}

	template <typename T, memory_safety Safety>
	basic_string<T, Safety> operator+(basic_string<T, Safety>&& a, typename basic_string<T, Safety>::value_type c)
	{
		a.push_back(c);
		return std::move(a);
	}

	template <typename T, memory_safety Safety>
	inline std::size_t basic_string<T, Safety>::hash() const noexcept
	{
		// To consider: limit p to at most 256 chars.
		auto p = begin_unsafe();
		auto e = end_unsafe();
		unsigned int result = 2166136261U;
//			while((c = *p++) != 0) // Using '!=' disables compiler warnings.
		for(; p != e; ++p) 
			result = (result * 16777619) ^ static_cast<unsigned int>(*p);
		return static_cast<std::size_t>(result);
	}

	template <typename T, memory_safety Safety>
	inline bool basic_string<T, Safety>::validate() const noexcept
	{
		if((internalLayout().BeginPtr() == nullptr) || (internalLayout().EndPtr() == nullptr))
			return false;
		if(internalLayout().EndPtr() < internalLayout().BeginPtr())
			return false;
		if(internalLayout().CapacityPtr() < internalLayout().EndPtr())
			return false;
		if(*internalLayout().EndPtr() != 0)
			return false;
		return true;
	}


	template <typename T, memory_safety Safety>
	inline detail::iterator_validity basic_string<T, Safety>::validate_iterator(const_pointer i) const noexcept
	{
		if(i == nullptr)
		 	return detail::iterator_validity::Null;
		else if(i >= internalLayout().BeginPtr())
		{
			if(i < internalLayout().EndPtr())
				return detail::iterator_validity::ValidCanDeref;

			else if(i == internalLayout().EndPtr())
				return detail::iterator_validity::ValidEnd;

			else if(i < internalLayout().CapacityPtr())
				return detail::iterator_validity::InvalidZoombie;
		}

		return detail::iterator_validity::xxx_Broken_xxx;
	}

	template <typename T, memory_safety Safety>
	inline detail::iterator_validity basic_string<T, Safety>::validate_iterator(csafe_it_arg i) const noexcept
	{
		if(i == const_iterator())
		 	return detail::iterator_validity::Null;
		else if(i.arr == internalLayout().GetSoftHeapPtr()) {

			const_pointer p = i.get_raw_ptr();
			if(p < internalLayout().EndPtr())
				return detail::iterator_validity::ValidCanDeref;

			else if(p == internalLayout().EndPtr())
				return detail::iterator_validity::ValidEnd;

			else if(p < internalLayout().CapacityPtr())
				return detail::iterator_validity::InvalidZoombie;
		}

		return detail::iterator_validity::InvalidZoombie;
	}

	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	// Operator== and operator!=
	template <typename T, memory_safety Safety>
	inline bool operator==(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return (a.size() == b.size()) && //(memcmp(a.data(), b.data(), (size_t)a.size() * sizeof(typename basic_string<T, Safety>::value_type)) == 0));
			(basic_string<T, Safety>::compare_unsafe(a.begin_unsafe(), a.end_unsafe(), b.begin_unsafe(), b.end_unsafe()) == 0);
	}


	template <typename T, memory_safety Safety>
	inline bool operator==(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		typedef typename basic_string<T, Safety>::size_type size_type;
		const T* p = l.c_str();
		const size_type n = (size_type)CharStrlen(p);
		return (n == b.size()) && //(memcmp(p, b.data(), (size_t)n * sizeof(*p)) == 0));
			(basic_string<T, Safety>::compare_unsafe(p, p + n, b.begin_unsafe(), b.end_unsafe()) == 0);
	}


	template <typename T, memory_safety Safety>
	inline bool operator==(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		typedef typename basic_string<T, Safety>::size_type size_type;
		const T* p = l.c_str();
		const size_type n = (size_type)CharStrlen(p);
		return (a.size() == n) && //(memcmp(a.data(), p, (size_t)n * sizeof(*p)) == 0));
			(basic_string<T, Safety>::compare_unsafe(a.begin_unsafe(), a.end_unsafe(), p, p + n) == 0);
	}


	template <typename T, memory_safety Safety>
	inline bool operator!=(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return !(a == b);
	}


	template <typename T, memory_safety Safety>
	inline bool operator!=(typename basic_string<T, Safety>::const_pointer p, const basic_string<T, Safety>& b)
	{
		return !(p == b);
	}


	template <typename T, memory_safety Safety>
	inline bool operator!=(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::const_pointer p)
	{
		return !(a == p);
	}


	// Operator< (and also >, <=, and >=).
	template <typename T, memory_safety Safety>
	inline bool operator<(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return basic_string<T, Safety>::compare_unsafe(a.begin_unsafe(), a.end_unsafe(), b.begin_unsafe(), b.end_unsafe()) < 0; }


	template <typename T, memory_safety Safety>
	inline bool operator<(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		typedef typename basic_string<T, Safety>::size_type size_type;
		const T* p = l.c_str();
		const size_type n = (size_type)CharStrlen(p);
		return basic_string<T, Safety>::compare_unsafe(p, p + n, b.begin_unsafe(), b.end_unsafe()) < 0;
	}


	template <typename T, memory_safety Safety>
	inline bool operator<(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		typedef typename basic_string<T, Safety>::size_type size_type;
		const T* p = l.c_str();
		const size_type n = (size_type)CharStrlen(p);
		return basic_string<T, Safety>::compare_unsafe(a.begin_unsafe(), a.end_unsafe(), p, p + n) < 0;
	}


	template <typename T, memory_safety Safety>
	inline bool operator>(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return b < a;
	}


	template <typename T, memory_safety Safety>
	inline bool operator>(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		return b < l;
	}


	template <typename T, memory_safety Safety>
	inline bool operator>(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		return l < a;
	}


	template <typename T, memory_safety Safety>
	inline bool operator<=(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return !(b < a);
	}


	template <typename T, memory_safety Safety>
	inline bool operator<=(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		return !(b < l);
	}


	template <typename T, memory_safety Safety>
	inline bool operator<=(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		return !(l < a);
	}


	template <typename T, memory_safety Safety>
	inline bool operator>=(const basic_string<T, Safety>& a, const basic_string<T, Safety>& b)
	{
		return !(a < b);
	}


	template <typename T, memory_safety Safety>
	inline bool operator>=(typename basic_string<T, Safety>::literal_type l, const basic_string<T, Safety>& b)
	{
		return !(l < b);
	}


	template <typename T, memory_safety Safety>
	inline bool operator>=(const basic_string<T, Safety>& a, typename basic_string<T, Safety>::literal_type l)
	{
		return !(a < l);
	}


	template <typename T, memory_safety Safety>
	inline void swap(basic_string<T, Safety>& a, basic_string<T, Safety>& b)
	{
		a.swap(b);
	}

	/// string / wstring
	typedef basic_string<char>    string;
	typedef basic_string<wchar_t> wstring;

	/// string8 / string16 / string32
	// typedef basic_string<char8_t>  string8;
	typedef basic_string<char16_t> string16;
	typedef basic_string<char32_t> string32;

	// C++11 string types
//	typedef basic_string<char8_t>  u8string;    // Actually not a C++11 type, but added for consistency.
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;


	/// hash<string>
	///
	/// We provide EASTL hash function objects for use in hash table containers.
	///
	/// Example usage:
	///    #include <EASTL/hash_set.h>
	///    hash_set<string> stringHashSet;
	///
//	template <typename T> struct hash;

	template <>
	struct hash<string>
	{
		std::size_t operator()(const string& x) const
		{
			return x.hash();
		}
	};

	template <>
	struct hash<string16>
	{
		std::size_t operator()(const string16& x) const
		{
			return x.hash();
		}
	};

	template <>
	struct hash<string32>
	{
		std::size_t operator()(const string32& x) const
		{
			return x.hash();
		}
	};

	// #if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
	template <>
	struct hash<wstring>
	{
		std::size_t operator()(const wstring& x) const
		{
			return x.hash();
		}
	};
	// #endif


	/// to_string
	///
	/// Converts integral types to an eastl::string with the same content that sprintf produces.  The following
	/// implementation provides a type safe conversion mechanism which avoids the common bugs associated with sprintf
	/// style format strings.
	///
	/// http://en.cppreference.com/w/cpp/string/basic_string/to_string
	///
	inline string to_string(int value)
		{ return string::snprintf_unsafe("%d", value); }
	inline string to_string(long value)
		{ return string::snprintf_unsafe("%ld", value); }
	inline string to_string(long long value)
		{ return string::snprintf_unsafe("%lld", value); }
	inline string to_string(unsigned value)
		{ return string::snprintf_unsafe("%u", value); }
	inline string to_string(unsigned long value)
		{ return string::snprintf_unsafe("%lu", value); }
	inline string to_string(unsigned long long value)
		{ return string::snprintf_unsafe("%llu", value); }
	inline string to_string(float value)
		{ return string::snprintf_unsafe("%f", value); }
	inline string to_string(double value)
		{ return string::snprintf_unsafe("%f", value); }
	inline string to_string(long double value)
		{ return string::snprintf_unsafe("%Lf", value); }


	/// to_wstring
	///
	/// Converts integral types to an eastl::wstring with the same content that sprintf produces.  The following
	/// implementation provides a type safe conversion mechanism which avoids the common bugs associated with sprintf
	/// style format strings.
	///
	/// http://en.cppreference.com/w/cpp/string/basic_string/to_wstring
	///
	inline wstring to_wstring(int value)
		{ return wstring::swprintf_unsafe(L"%d", value); }
	inline wstring to_wstring(long value)
		{ return wstring::swprintf_unsafe(L"%ld", value); }
	inline wstring to_wstring(long long value)
		{ return wstring::swprintf_unsafe(L"%lld", value); }
	inline wstring to_wstring(unsigned value)
		{ return wstring::swprintf_unsafe(L"%u", value); }
	inline wstring to_wstring(unsigned long value)
		{ return wstring::swprintf_unsafe(L"%lu", value); }
	inline wstring to_wstring(unsigned long long value)
		{ return wstring::swprintf_unsafe(L"%llu", value); }
	inline wstring to_wstring(float value)
		{ return wstring::swprintf_unsafe(L"%f", value); }
	inline wstring to_wstring(double value)
		{ return wstring::swprintf_unsafe(L"%f", value); }
	inline wstring to_wstring(long double value)
		{ return wstring::swprintf_unsafe(L"%Lf", value); }


	/// user defined literals
	///
	/// Converts a character array literal to a basic_string.
	///
	/// Example:
	///   string s = "abcdef"s;
	///
	/// http://en.cppreference.com/w/cpp/string/basic_string/operator%22%22s
	///
	// #if EASTL_USER_LITERALS_ENABLED && EASTL_INLINE_NAMESPACES_ENABLED
	// 	EA_DISABLE_VC_WARNING(4455) // disable warning C4455: literal suffix identifiers that do not start with an underscore are reserved
	//     inline namespace literals
	//     {
	// 	    inline namespace string_literals
	// 	    {
	// 			inline string operator"" s(const char* str, size_t len) noexcept { return {str, string::size_type(len)}; }
	// 			inline u16string operator"" s(const char16_t* str, size_t len) noexcept { return {str, u16string::size_type(len)}; }
	// 			inline u32string operator"" s(const char32_t* str, size_t len) noexcept { return {str, u32string::size_type(len)}; }
	// 			inline wstring operator"" s(const wchar_t* str, size_t len) noexcept { return {str, wstring::size_type(len)}; }
	// 	    }
	//     }
	// 	EA_RESTORE_VC_WARNING()  // warning: 4455
	// #endif

} // namespace safe_memory


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#endif // Header include guard
