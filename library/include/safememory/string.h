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

#ifndef SAFE_MEMORY_STRING_H
#define SAFE_MEMORY_STRING_H

#include <EASTL/string.h>
#include <safememory/detail/allocator_to_eastl.h>
#include <safememory/detail/array_iterator.h>
#include <safememory/string_literal.h>
#include <safememory/functional.h> //for hash

namespace safememory
{

	template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS basic_string : protected eastl::basic_string<T, detail::allocator_to_eastl_string<Safety>>
	{
	public:
		typedef basic_string<T, Safety>                         this_type;
		typedef eastl::basic_string<T, detail::allocator_to_eastl_string<Safety>>  base_type;
        
		typedef basic_string_literal<T>                         literal_type;
		typedef typename base_type::heap_array_type             heap_array_type;

		typedef typename base_type::value_type                  value_type;
		typedef typename base_type::pointer                     pointer;
		typedef typename base_type::const_pointer               const_pointer;
		typedef typename base_type::reference                   reference;
		typedef typename base_type::const_reference             const_reference;
		typedef typename base_type::iterator                    iterator_base;
		typedef typename base_type::const_iterator              const_iterator_base;
		typedef typename base_type::reverse_iterator            reverse_iterator_base;
		typedef typename base_type::const_reverse_iterator      const_reverse_iterator_base;

		typedef typename base_type::size_type                   size_type;
		typedef typename base_type::difference_type             difference_type;
		typedef typename base_type::allocator_type              allocator_type;


		typedef typename detail::array_of_iterator_raw<T>               stack_only_iterator;
		typedef typename detail::const_array_of_iterator_raw<T>         const_stack_only_iterator;
		typedef typename detail::array_of_iterator_soft_ptr<T, Safety>        heap_safe_iterator;
		typedef typename detail::const_array_of_iterator_soft_ptr<T, Safety>  const_heap_safe_iterator;

		// mb: for 'memory_safety::none' we boil down to use the base (eastl) iterator
		// or use the same iterator as 'safe' but passing the 'memory_safety::none' parameter
		// down the line 
		static constexpr bool use_base_iterator = allocator_type::use_base_iterator;
		
		typedef std::conditional_t<use_base_iterator, iterator_base, stack_only_iterator>               iterator;
		typedef std::conditional_t<use_base_iterator, const_iterator_base, const_stack_only_iterator>   const_iterator;
		typedef std::conditional_t<use_base_iterator, reverse_iterator_base,
									eastl::reverse_iterator<iterator>>                                  reverse_iterator;
		typedef std::conditional_t<use_base_iterator, const_reverse_iterator_base,
									eastl::reverse_iterator<const_iterator>>                            const_reverse_iterator;

		typedef heap_safe_iterator                iterator_safe;
		typedef const_heap_safe_iterator    const_iterator_safe;
		typedef eastl::reverse_iterator<iterator_safe>                             reverse_iterator_safe;
		typedef eastl::reverse_iterator<const_iterator_safe>                       const_reverse_iterator_safe;


		template<typename>
		friend struct hash;

        using base_type::npos;
		static constexpr memory_safety is_safe = Safety;

	public:
		// CtorDoNotInitialize exists so that we can create a constructor that allocates but doesn't
		// initialize and also doesn't collide with any other constructor declaration.
		struct CtorDoNotInitialize{};

		// CtorSprintf exists so that we can create a constructor that accepts printf-style
		// arguments but also doesn't collide with any other constructor declaration.
		struct CtorSprintf{};

		// CtorConvert exists so that we can have a constructor that implements string encoding
		// conversion, such as between UCS2 char16_t and UTF8 char8_t.
		struct CtorConvert{};

		// #ifdef EA_SYSTEM_BIG_ENDIAN
		// 	static constexpr size_type kMaxSize = (~kHeapMask) >> 1;
		// #else
		// 	static constexpr size_type kMaxSize = ~kHeapMask;
		// #endif

	protected:
		struct CtorBaseType{};
		basic_string(CtorBaseType, const base_type& b) : base_type(b) {}
		basic_string(CtorBaseType, base_type&& b) : base_type(std::move(b)) {}
	public:
		// Constructor, destructor
		basic_string() : base_type(allocator_type()) {}
		// explicit basic_string(const allocator_type& allocator) EA_NOEXCEPT;
		basic_string(const this_type& x, size_type position, size_type n = npos) : base_type(x, x.checkPos(position), n) {}
		// basic_string(const value_type* p, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
		// EASTL_STRING_EXPLICIT basic_string(const value_type* p, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
		basic_string(const value_type* p) : base_type(p, allocator_type()) {}
		basic_string(const literal_type& l) : base_type(l.c_str(), l.size(), allocator_type()) {}
		basic_string(size_type n, value_type c) : base_type(n, c, allocator_type()) {}
		basic_string(const this_type& x) = default;
	    // basic_string(const this_type& x, const allocator_type& allocator);
		// basic_string(const value_type* pBegin, const value_type* pEnd, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
		// basic_string(CtorDoNotInitialize, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
		// basic_string(CtorSprintf, const value_type* pFormat, ...);
		basic_string(std::initializer_list<value_type> init) : base_type(init, allocator_type()) {}

		basic_string(this_type&& x) = default;
		// basic_string(this_type&& x, const allocator_type& allocator);

		// explicit basic_string(const view_type& sv, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);
		// basic_string(const view_type& sv, size_type position, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);

		// template <typename OtherCharType>
		// basic_string(CtorConvert, const OtherCharType* p, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);

		// template <typename OtherCharType>
		// basic_string(CtorConvert, const OtherCharType* p, size_type n, const allocator_type& allocator = EASTL_BASIC_STRING_DEFAULT_ALLOCATOR);

		// template <typename OtherStringType> // Unfortunately we need the CtorConvert here because otherwise this function would collide with the value_type* constructor.
		// basic_string(CtorConvert, const OtherStringType& x);

	   ~basic_string() {}

		// Implicit conversion operator
		// operator basic_string_view<T>() const EA_NOEXCEPT;

		// Operator=
		this_type& operator=(const this_type& x) = default;
		this_type& operator=(const value_type* p) { base_type::operator=(p); return *this; }
		this_type& operator=(const literal_type& l) { base_type::assign(l.c_str(), l.size()); return *this; }
		this_type& operator=(value_type c) { base_type::operator=(c); return *this; }
		this_type& operator=(std::initializer_list<value_type> ilist) { base_type::operator=(ilist); return *this; }
		// this_type& operator=(view_type v);
		this_type& operator=(this_type&& x) = default;

		// #if EASTL_OPERATOR_EQUALS_OTHER_ENABLED
		// 	this_type& operator=(value_type* p) { return operator=((const value_type*)p); } // We need this because otherwise the const value_type* version can collide with the const OtherStringType& version below.

		// 	template <typename OtherCharType>
		// 	this_type& operator=(const OtherCharType* p);

		// 	template <typename OtherStringType>
		// 	this_type& operator=(const OtherStringType& x);
		// #endif

		void swap(this_type& x) { base_type::swap(x); }

		// Assignment operations
		this_type& assign(const this_type& x) { base_type::assign(x); return *this; }
		this_type& assign(const this_type& x, size_type position, size_type n = npos) {
            x.checkPos(position);
            base_type::assign(x, position, n);
            return *this;
        }
		this_type& assign_unsafe(const value_type* p, size_type n) { base_type::assign(p, n); return *this; }
		this_type& assign_unsafe(const value_type* p) { base_type::assign(p); return *this; }
		this_type& assign(const value_type* p) { base_type::assign(p); return *this; }
		this_type& assign(const literal_type& l) { base_type::assign(l.c_str(), l.size()); return *this; }
		this_type& assign(size_type n, value_type c) { base_type::assign(n, c); return *this; }
		this_type& assign_unsafe(const value_type* pBegin, const value_type* pEnd) { base_type::assign(pBegin, pEnd); return *this; }
		this_type& assign(this_type&& x) { base_type::assign(std::move(x)); return *this; }
		this_type& assign(std::initializer_list<value_type> init) { base_type::assign(init); return *this; }

		template <typename OtherCharType>
		this_type& assign_convert_unsafe(const OtherCharType* p) { base_type::assign_convert(p); return *this; }

		template <typename OtherCharType>
		this_type& assign_convert_unsafe(const OtherCharType* p, size_type n) { base_type::assign_convert(p, n); return *this; }

		template <typename OtherStringType>
		this_type& assign_convert_unsafe(const OtherStringType& x) { base_type::assign_convert(x); return *this; }

		// Iterators.
		iterator_base       begin_unsafe() noexcept { return base_type::begin(); }
		const_iterator_base begin_unsafe() const noexcept { return base_type::begin(); }
		const_iterator_base cbegin_unsafe() const noexcept { return base_type::cbegin(); }

		iterator_base       end_unsafe() noexcept { return base_type::end(); }
		const_iterator_base end_unsafe() const noexcept { return base_type::end(); }
		const_iterator_base cend_unsafe() const noexcept { return base_type::cend(); }


		iterator       begin() noexcept { return makeIt(base_type::begin()); }
		const_iterator begin() const noexcept { return makeIt(base_type::begin()); }
		const_iterator cbegin() const noexcept { return makeIt(base_type::cbegin()); }

		iterator       end() noexcept { return makeIt(base_type::end()); }
		const_iterator end() const noexcept { return makeIt(base_type::end()); }
		const_iterator cend() const noexcept { return makeIt(base_type::cend()); }

		reverse_iterator       rbegin() noexcept { return makeIt(base_type::rbegin()); }
		const_reverse_iterator rbegin() const noexcept { return makeIt(base_type::rbegin()); }
		const_reverse_iterator crbegin() const noexcept { return makeIt(base_type::crbegin()); }

		reverse_iterator       rend() noexcept { return makeIt(base_type::rend()); }
		const_reverse_iterator rend() const noexcept { return makeIt(base_type::rend()); }
		const_reverse_iterator crend() const noexcept { return makeIt(base_type::crend()); }

		iterator_safe       begin_safe() noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator_safe begin_safe() const noexcept { return makeSafeIt(base_type::begin()); }
		const_iterator_safe cbegin_safe() const noexcept { return makeSafeIt(base_type::cbegin()); }

		iterator_safe       end_safe() noexcept { return makeSafeIt(base_type::end()); }
		const_iterator_safe end_safe() const noexcept { return makeSafeIt(base_type::end()); }
		const_iterator_safe cend_safe() const noexcept { return makeSafeIt(base_type::cend()); }

		reverse_iterator_safe       rbegin_safe() noexcept { return makeSafeIt(base_type::rbegin()); }
		const_reverse_iterator_safe rbegin_safe() const noexcept { return makeSafeIt(base_type::rbegin()); }
		const_reverse_iterator_safe crbegin_safe() const noexcept { return makeSafeIt(base_type::crbegin()); }

		reverse_iterator_safe       rend_safe() noexcept { return makeSafeIt(base_type::rend()); }
		const_reverse_iterator_safe rend_safe() const noexcept { return makeSafeIt(base_type::rend()); }
		const_reverse_iterator_safe crend_safe() const noexcept { return makeSafeIt(base_type::crend()); }

		// Size-related functionality
        using base_type::empty;
        using base_type::size;
        using base_type::length;
        using base_type::max_size;
        using base_type::capacity;
        using base_type::resize;
        using base_type::reserve;
        using base_type::set_capacity;
        using base_type::force_size; //TODO: review
        using base_type::shrink_to_fit;

		// Raw access
        using base_type::data;
        using base_type::c_str;

		// Element access
		reference       operator[](size_type n) {  return at(n); }
		const_reference operator[](size_type n) const {  return at(n); }

		reference       at(size_type n) {
            checkPos(n);
            return base_type::at(n);
        }

		const_reference at(size_type n) const {
            checkPos(n);
            return base_type::at(n);
        }

        using base_type::front;
        using base_type::back;

		// Append operations
		this_type& operator+=(const this_type& x) { base_type::operator+=(x); return *this; }
		this_type& operator+=(const value_type* p) { base_type::operator+=(p); return *this; }
		this_type& operator+=(const literal_type& l) { base_type::append(l.c_str(), l.size()); return *this; }
		this_type& operator+=(value_type c) { base_type::operator+=(c); return *this; }

		this_type& append(const this_type& x) { base_type::append(x); return *this; }
		this_type& append(const this_type& x,  size_type position, size_type n = npos) {
            checkPos(position);
            base_type::append(x, position, n);
            return *this;
        }
		// this_type& append(const value_type* p, size_type n);
		this_type& append(const value_type* p) { base_type::append(p); return *this; }
		this_type& append(const literal_type& l) { base_type::append(l.c_str(), l.size()); return *this; }
		this_type& append(size_type n, value_type c) { base_type::append(n, c); return *this; }
		this_type& append_unsafe(const value_type* pBegin, const value_type* pEnd) { base_type::append(pBegin, pEnd); return *this; }

		// this_type& append_sprintf_va_list(const value_type* pFormat, va_list arguments);
		// this_type& append_sprintf(const value_type* pFormat, ...);
 
		template <typename OtherCharType>
		this_type& append_convert_unsafe(const OtherCharType* p) {
			base_type::append_convert(p);
			return *this;
		}

		template <typename OtherCharType>
		this_type& append_convert_unsafe(const OtherCharType* p, size_type n) {
			base_type::append_convert(p, n);
			return *this;
		}

		template <typename OtherStringType>
		this_type& append_convert_unsafe(const OtherStringType& x) {
			base_type::append_convert(x);
			return *this;
		}

		template <typename OtherCharType>
		this_type& append_convert(const basic_string_literal<OtherCharType>& l) {
			base_type::append_convert(l.c_str());
			return *this;
		}

		template <typename OtherCharType> // only same Safety by now
		this_type& append_convert(const basic_string<OtherCharType, Safety>& x) {
			base_type::append_convert(x);
			return *this;
		}

		
        using base_type::push_back;

		void pop_back() {
			if constexpr (Safety == memory_safety::safe) {
				if(NODECPP_UNLIKELY(empty())) {
					ThrowRangeException("basic_string::pop_back -- empty string");
				}
			}
			base_type::pop_back();
		}

		// Insertion operations
		this_type& insert(size_type position, const this_type& x) {
            checkPos(position);
            base_type::insert(position, x);
            return *this;
        }

		this_type& insert(size_type position, const this_type& x, size_type beg, size_type n) {
            checkPos(position);
            x.checkPos(beg);
            base_type::insert(position, x, beg, n);
            return *this;
        }

		// this_type& insert(size_type position, const value_type* p, size_type n);
		this_type& insert(size_type position, const value_type* p) {
            checkPos(position);
            base_type::insert(position, p);
            return *this;
        }

		this_type& insert(size_type position, const literal_type& l) {
            checkPos(position);
            base_type::insert(position, l.c_str(), l.size());
            return *this;
        }

		this_type& insert(size_type position, size_type n, value_type c) {
            checkPos(position);
            base_type::insert(position, n, c);
            return *this;
        }

		iterator   insert(const_iterator p, value_type c) { return makeIt(base_type::insert(toBase(p), c)); }
		iterator   insert(const_iterator p, size_type n, value_type c) { return makeIt(base_type::insert(toBase(p), n, c)); }
		// iterator   insert(const_iterator p, const value_type* pBegin, const value_type* pEnd);
		iterator   insert(const_iterator p, std::initializer_list<value_type> init) { return makeIt(base_type::insert(toBase(p), init)); }


		iterator_safe   insert_safe(const_iterator_safe p, value_type c) { return makeSafeIt(base_type::insert(toBase(p), c)); }
		iterator_safe   insert_safe(const_iterator_safe p, size_type n, value_type c) { return makeSafeIt(base_type::insert(toBase(p), n, c)); }
		// iterator   insert(const_iterator p, const value_type* pBegin, const value_type* pEnd);
		iterator_safe   insert_safe(const_iterator_safe p, std::initializer_list<value_type> init) { return makeSafeIt(base_type::insert(toBase(p), init)); }

		// Erase operations
		this_type&       erase(size_type position = 0, size_type n = npos) {
            checkPos(position);
            base_type::erase(position, n);
            return *this;
        }

		iterator         erase(const_iterator p) { return makeIt(base_type::erase(toBase(p))); }
        iterator         erase(const_iterator pBegin, const_iterator pEnd) {
            auto p = toBase(pBegin, pEnd);
            return makeIt(base_type::erase(p.first, p.second));
        }

		iterator_safe         erase_safe(const_iterator_safe p) { return makeSafeIt(base_type::erase(toBase(p))); }
        iterator_safe         erase_safe(const_iterator_safe pBegin, const_iterator pEnd) {
            auto p = toBase(pBegin, pEnd);
            return makeSafeIt(base_type::erase(p.first, p.second));
        }

		// reverse_iterator erase(reverse_iterator position);
		// reverse_iterator erase(reverse_iterator first, reverse_iterator last);
        using base_type::clear;

		// // Detach memory
		// pointer detach() EA_NOEXCEPT;

		// Replacement operations
		this_type&  replace(size_type position, size_type n,  const this_type& x) {
            checkPos(position);
            base_type::replace(position, n, x);
            return *this;
        }

		this_type&  replace(size_type pos1,     size_type n1, const this_type& x,  size_type pos2, size_type n2 = npos) {
            checkPos(pos1);
            x.checkPos(pos2);
            base_type::replace(pos1, n1, x, pos2, n2);
            return *this;
        }

		// this_type&  replace(size_type position, size_type n1, const value_type* p, size_type n2);
		this_type&  replace(size_type position, size_type n1, const value_type* p) {
            checkPos(position);
            base_type::replace(position, n1, p);
            return *this;
        }

		this_type&  replace(size_type position, size_type n1, const literal_type& l) {
            checkPos(position);
            base_type::replace(position, n1, l.c_str(), l.size());
            return *this;
        }

		this_type&  replace(size_type position, size_type n1, size_type n2, value_type c) {
            checkPos(position);
            base_type::replace(position, n1, n2, c);
            return *this;
        }

		this_type&  replace(const_iterator first, const_iterator last, const this_type& x) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, x);
            return *this;
        }

		// this_type&  replace(const_iterator first, const_iterator last, const value_type* p, size_type n) {}
		this_type&  replace(const_iterator first, const_iterator last, const value_type* p) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, p);
            return *this;
        }

		this_type&  replace(const_iterator first, const_iterator last, const literal_type& l) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, l.c_str(), l.size());
            return *this;
        }

		this_type&  replace(const_iterator first, const_iterator last, size_type n, value_type c) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, n, c);
            return *this;
        }

		this_type&  replace_safe(const_iterator_safe first, const_iterator_safe last, const this_type& x) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, x);
            return *this;
        }

		this_type&  replace_safe(const_iterator_safe first, const_iterator_safe last, const value_type* p) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, p);
            return *this;
        }

		this_type&  replace_safe(const_iterator_safe first, const_iterator_safe last, const literal_type& l) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, l.c_str(), l.size());
            return *this;
        }

		this_type&  replace_safe(const_iterator_safe first, const_iterator_safe last, size_type n, value_type c) {
            auto p = toBase(first, last);
            base_type::replace(p.first, p.second, n, c);
            return *this;
        }

		// this_type&  replace(const_iterator first, const_iterator last, const value_type* pBegin, const value_type* pEnd);
		// size_type   copy(value_type* p, size_type n, size_type position = 0) const;

		// Find operations
		size_type find(const this_type& x,  size_type position = 0) const noexcept { return base_type::find(x.toBase(), position); }
		size_type find(const value_type* p, size_type position = 0) const { return base_type::find(p, position); }
		// size_type find(const value_type* p, size_type position, size_type n);
		size_type find(value_type c, size_type position = 0) const noexcept { return base_type::find(c, position); }
		size_type find(const literal_type& l,  size_type position = 0) const noexcept { return base_type::find(l.c_str(), position, l.size()); }

		// Reverse find operations
		size_type rfind(const this_type& x,  size_type position = npos) const noexcept { return base_type::rfind(x.toBase(), position); }
		size_type rfind(const value_type* p, size_type position = npos) const { return base_type::rfind(p, position); }
		// size_type rfind(const value_type* p, size_type position, size_type n);
		size_type rfind(value_type c, size_type position = npos) const noexcept { return base_type::rfind(c, position); }
		size_type rfind(const literal_type& l,  size_type position = 0) const noexcept { return base_type::rfind(l.c_str(), position, l.size()); }

		// Find first-of operations
		size_type find_first_of(const this_type& x, size_type position = 0) const noexcept { return base_type::find_first_of(x.toBase(), position); }
		size_type find_first_of(const value_type* p, size_type position = 0) const { return base_type::find_first_of(p, position); }
		// size_type find_first_of(const value_type* p, size_type position, size_type n);
		size_type find_first_of(value_type c, size_type position = 0) const noexcept { return base_type::find_first_of(c, position); }
		size_type find_first_of(const literal_type& l,  size_type position = 0) const noexcept { return base_type::find_first_of(l.c_str(), position, l.size()); }

		// Find last-of operations
		size_type find_last_of(const this_type& x, size_type position = npos) const noexcept { return base_type::find_last_of(x.toBase(), position); }
		size_type find_last_of(const value_type* p, size_type position = npos) const { return base_type::find_last_of(p, position); }
		// size_type find_last_of(const value_type* p, size_type position, size_type n);
		size_type find_last_of(value_type c, size_type position = npos) const noexcept { return base_type::find_last_of(c, position); }
		size_type find_last_of(const literal_type& l,  size_type position = 0) const noexcept { return base_type::find_last_of(l.c_str(), position, l.size()); }

		// Find first not-of operations
		size_type find_first_not_of(const this_type& x, size_type position = 0) const noexcept { return base_type::find_first_not_of(x.toBase(), position); }
		size_type find_first_not_of(const value_type* p, size_type position = 0) const { return base_type::find_first_not_of(p, position); }
		// size_type find_first_not_of(const value_type* p, size_type position, size_type n);
		size_type find_first_not_of(value_type c, size_type position = 0) const noexcept { return base_type::find_first_not_of(c, position); }
		size_type find_first_not_of(const literal_type& l,  size_type position = 0) const noexcept { return base_type::find_first_not_of(l.c_str(), position, l.size()); }

		// Find last not-of operations
		size_type find_last_not_of(const this_type& x,  size_type position = npos) const noexcept { return base_type::find_last_not_of(x.toBase(), position); }
		size_type find_last_not_of(const value_type* p, size_type position = npos) const { return base_type::find_last_not_of(p, position); }
		// size_type find_last_not_of(const value_type* p, size_type position, size_type n);
		size_type find_last_not_of(value_type c, size_type position = npos) const noexcept { return base_type::find_last_not_of(c, position); }
		size_type find_last_not_of(const literal_type& l,  size_type position = 0) const noexcept { return base_type::find_last_not_of(l.c_str(), position, l.size()); }

		// Substring functionality
		this_type substr(size_type position = 0, size_type n = npos) const {
            checkPos(position);
            return {CtorBaseType(), base_type::substr(position, n)};
        }

		// Comparison operations
		int        compare(const this_type& x) const noexcept { return base_type::compare(x); }
		int        compare(size_type pos1, size_type n1, const this_type& x) const {
            checkPos(pos1);
            return base_type::compare(pos1, n1, x);
        }

		int        compare(size_type pos1, size_type n1, const this_type& x, size_type pos2, size_type n2) const {
            checkPos(pos1);
            x.checkPos(pos2);
            return base_type::compare(pos1, n1, x, pos2, n2);
        }

		// int        compare(size_type pos1, size_type n1, const value_type* p, size_type n2) const;
		int        compare(const value_type* p) const { return base_type::compare(p); }

		int        compare(size_type pos1, size_type n1, const value_type* p) const {
            checkPos(pos1);
            return base_type::compare(pos1, n1, p);
        }

		int        compare(size_type pos1, size_type n1, const literal_type& l) const {
            checkPos(pos1);
            return base_type::compare(pos1, n1, l.c_str(), l.size());
        }

		// static int compare(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

		// Case-insensitive comparison functions. Not part of C++ this_type. Only ASCII-level locale functionality is supported. Thus this is not suitable for localization purposes.
		int        comparei(const this_type& x) const noexcept { return base_type::comparei(x); }
		int        comparei(const value_type* p) const { return base_type::comparei(p); }
		int        comparei(const literal_type& l) const { return base_type::comparei(l.c_str()); }
		// static int comparei(const value_type* pBegin1, const value_type* pEnd1, const value_type* pBegin2, const value_type* pEnd2);

		// Misc functionality, not part of C++ this_type.
        using base_type::make_lower;
        using base_type::make_upper;
		void         ltrim() { base_type::ltrim(); }
		void         rtrim() { base_type::rtrim(); }
		void         trim() { base_type::trim(); }
		void         ltrim(const value_type* p) { base_type::ltrim(p); }
		void         rtrim(const value_type* p) { base_type::rtrim(p); }
		void         trim(const value_type* p) { base_type::trim(p); }
		void         ltrim(const literal_type& l) { base_type::ltrim(l.c_str()); }
		void         rtrim(const literal_type& l) { base_type::rtrim(l.c_str()); }
		void         trim(const literal_type& l) { base_type::trim(l.c_str()); }
		this_type    left(size_type n) const { return {CtorBaseType(), base_type::left(n)}; }
		this_type    right(size_type n) const { return {CtorBaseType(), base_type::right(n)}; }
		// this_type&   sprintf_va_list(const value_type* pFormat, va_list arguments);
		// this_type&   sprintf(const value_type* pFormat, ...);

		using base_type::validate;
		int validate_iterator(const_iterator_base it) const noexcept { return base_type::validate_iterator(it); }
		//TODO: custom validation for safe iterators
		int validate_iterator(const const_stack_only_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }
		int validate_iterator(const const_heap_safe_iterator& it) const noexcept { return base_type::validate_iterator(toBase(it)); }

		bool operator==(const this_type& b)	const { return eastl::operator==(this->toBase(), b.toBase()); }
		bool operator==(const literal_type& l) const { return eastl::operator==(this->toBase(), l.c_str()); }
		bool operator==(const value_type* p) const { return eastl::operator==(this->toBase(), p); }
		bool operator!=(const this_type& b) const { return eastl::operator!=(this->toBase(), b.toBase()); }
		bool operator!=(const literal_type& l) const { return eastl::operator!=(this->toBase(), l.c_str()); }
		bool operator!=(const value_type* p) const { return eastl::operator!=(this->toBase(), p); }
		bool operator<(const this_type& b) const { return eastl::operator<(this->toBase(), b.toBase()); }
		bool operator<(const literal_type& l) const { return eastl::operator<(this->toBase(), l.c_str()); }
		bool operator<(const value_type* p) const { return eastl::operator<(this->toBase(), p); }
		bool operator>(const this_type& b) const { return eastl::operator>(this->toBase(), b.toBase()); }
		bool operator>(const literal_type& l) const { return eastl::operator>(this->toBase(), l.c_str()); }
		bool operator>(const value_type* p) const { return eastl::operator>(this->toBase(), p); }
		bool operator<=(const this_type& b) const { return eastl::operator<=(this->toBase(), b.toBase()); }
		bool operator<=(const literal_type& l) const { return eastl::operator<=(this->toBase(), l.c_str()); }
		bool operator<=(const value_type* p) const { return eastl::operator<=(this->toBase(), p); }
		bool operator>=(const this_type& b) const { return eastl::operator>=(this->toBase(), b.toBase()); }
		bool operator>=(const literal_type& l) const { return eastl::operator>=(this->toBase(), l.c_str()); }
		bool operator>=(const value_type* p) const { return eastl::operator>=(this->toBase(), p); }


		iterator_safe make_safe(const iterator& it) const {	return makeSafeIt(toBase(it)); }
		const_iterator_safe make_safe(const const_iterator& it) const {	return makeSafeIt(toBase(it)); }

    protected:
		[[noreturn]] static void ThrowRangeException(const char* msg) { throw std::out_of_range(msg); }
		[[noreturn]] static void ThrowInvalidArgumentException(const char* msg) { throw std::invalid_argument(msg); }

        const base_type& toBase() const noexcept { return *this; }

		// all 'toBase' are called with iterators coming from user, and its validity is unknown
		// so they must be checked

		// Safety == none
		const_iterator_base toBase(const_iterator_base it) const { return it; }
		std::pair<const_iterator_base, const_iterator_base> toBase(const_iterator_base it, const_iterator_base it2) const {
			return { it, it2 };
		}
		
		// Safety == safe
		const_iterator_base toBase(const const_stack_only_iterator& it) const {
			return it.toRaw(base_type::begin());
		}

		std::pair<const_iterator_base, const_iterator_base> toBase(const const_stack_only_iterator& it, const const_stack_only_iterator& it2) const {
			return it.toRaw(base_type::begin(), it2);
		}

		const_iterator_base toBase(const const_heap_safe_iterator& it) const {
			return it.toRaw(base_type::begin());
		}

		std::pair<const_iterator_base, const_iterator_base> toBase(const const_heap_safe_iterator& it, const const_heap_safe_iterator& it2) const {
			return it.toRaw(base_type::begin(), it2);
		}

        size_type checkPos(size_type position) const {
            // mb: when EASTL_STRING_OPT_RANGE_ERRORS is 1, position is already checked at
            // eastl::basic_string. However, we prefer to check ourselves depending on
            // Safety template parameter
            #if !EASTL_STRING_OPT_RANGE_ERRORS
                if constexpr (Safety == memory_safety::safe) {
                    if(NODECPP_UNLIKELY(position > size())) {
                        ThrowInvalidArgumentException("vector -- invalid argument");
                    }
                }
            #endif
            return position;
        }

		// all 'makeIt' and 'makeSafeIt' are called with iterators coming from eastl::string
		// or iterators already validated, so we don't do any checks in here

		iterator makeIt(iterator_base it) {
			if constexpr (use_base_iterator)
				return it;
			else
				return iterator::makePtr(base_type::data(), it, base_type::capacity());
		}
		
		const_iterator makeIt(const_iterator_base it) const {
			if constexpr (use_base_iterator)
				return it;
			else
				return const_iterator::makePtr(const_cast<T*>(base_type::data()), it, base_type::capacity());
		}

		reverse_iterator makeIt(const reverse_iterator_base& it) {
			if constexpr (use_base_iterator)
				return it;
			else
				return reverse_iterator(makeIt(it.base()));
		}
		const_reverse_iterator makeIt(const const_reverse_iterator_base& it) const {
			if constexpr (use_base_iterator)
				return it;
			else
				return const_reverse_iterator(makeIt(it.base()));
		}

		//mb: in case string is usign SSO, we make a 'reserve' to force switch to heap
		iterator_safe makeSafeIt(iterator_base it) {
			// first calculate index of 'it', in case we move to heap
			auto ix = static_cast<size_type>(it - base_type::data());

			if(base_type::internalLayout().IsSSO()) {
				// its on the stack, move it to heap
				base_type::reserve(base_type::SSOLayout::SSO_CAPACITY + 1);
			}
			
			//mb: now the buffer should be on the heap
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::regular, base_type::internalLayout().IsHeap());
			return iterator_safe::makeIx(allocator_type::to_soft(base_type::internalLayout().GetHeapBeginPtr()), ix, base_type::capacity());
		}

		//mb: in case string is usign SSO, we make a 'reserve' to force switch to heap
		const_iterator_safe makeSafeIt(const_iterator_base it) const {
			// first calculate index of 'it', in case we move to heap
			auto ix = static_cast<size_type>(it - base_type::data());

			if(base_type::internalLayout().IsSSO()) {
				// its on the stack, move it to heap
				const_cast<this_type*>(this)->base_type::reserve(base_type::SSOLayout::SSO_CAPACITY + 1);
			}

			//mb: now the buffer should be on the heap
			NODECPP_ASSERT(safememory::module_id, nodecpp::assert::AssertLevel::regular, base_type::internalLayout().IsHeap());
			return const_iterator_safe::makeIx(allocator_type::to_soft(base_type::internalLayout().GetHeapBeginPtr()), ix, base_type::capacity());
		}

		reverse_iterator_safe makeSafeIt(const reverse_iterator_base& it) {
			return reverse_iterator_safe(makeSafeIt(it.base()));
		}

		const_reverse_iterator_safe makeSafeIt(const const_reverse_iterator_base& it) const {
			return const_reverse_iterator_safe(makeSafeIt(it.base()));
		}

	}; // basic_string

	// non members operators, defined in terms of members ones
	// to avoid having a lot of friends

	template <typename T, memory_safety Safety>
	inline bool operator==(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator==(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator==(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator==(ptr);
	}

	template <typename T, memory_safety Safety>
	inline bool operator!=(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator!=(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator!=(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator!=(ptr);
	}

	template <typename T, memory_safety Safety>
	inline bool operator<(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator>(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator<(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator>(ptr);
	}

	template <typename T, memory_safety Safety>
	inline bool operator>(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator<(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator>(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator<(ptr);
	}

	template <typename T, memory_safety Safety>
	inline bool operator<=(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator>=(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator<=(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator>=(ptr);
	}

	template <typename T, memory_safety Safety>
	inline bool operator>=(const typename basic_string<T, Safety>::literal_type& lit, const basic_string<T, Safety>& str) {
        return str.operator<=(lit);
	}

	template <typename T, memory_safety Safety>
	inline bool operator>=(const typename basic_string<T, Safety>::value_type* ptr, const basic_string<T, Safety>& str) {
        return str.operator<=(ptr);
	}

	template <typename T, memory_safety Safety>
	inline void swap(basic_string<T, Safety>& a, basic_string<T, Safety>& b) {
		a.swap(b); 
	}


	/// string / wstring
	typedef basic_string<char>    string;
	typedef basic_string<wchar_t> wstring;

	/// custom string8 / string16 / string32
	typedef basic_string<char>     string8;
	typedef basic_string<char16_t> string16;
	typedef basic_string<char32_t> string32;

	/// ISO mandated string types
	typedef basic_string<char8_t>  u8string;    // Actually not a C++11 type, but added for consistency.
	typedef basic_string<char16_t> u16string;
	typedef basic_string<char32_t> u32string;


	template<typename T>
	struct SAFEMEMORY_DEEP_CONST hash<basic_string<T, memory_safety::none>> : eastl::hash<typename safememory::basic_string<T, safememory::memory_safety::none>::base_type>
	{
		typedef eastl::hash<typename safememory::basic_string<T, safememory::memory_safety::none>::base_type> base_type;
		SAFEMEMORY_NO_SIDE_EFFECT size_t operator()(const basic_string<T, memory_safety::none>& x) const
		{
			return base_type::operator()(x.toBase());
		}
	};

	template<typename T>
	struct SAFEMEMORY_DEEP_CONST hash<basic_string<T, memory_safety::safe>> : eastl::hash<typename safememory::basic_string<T, safememory::memory_safety::safe>::base_type>
	{
		typedef eastl::hash<typename safememory::basic_string<T, safememory::memory_safety::safe>::base_type> base_type;
		SAFEMEMORY_NO_SIDE_EFFECT size_t operator()(const basic_string<T, memory_safety::safe>& x) const
		{
			return base_type::operator()(x.toBase());
		}
	};


	template <typename T, memory_safety Safety = safeness_declarator<T>::is_safe>
	class SAFEMEMORY_DEEP_CONST_WHEN_PARAMS basic_string_safe : public basic_string<T, Safety>
	{
	public:
		typedef basic_string_safe<T, Safety>                    this_type;
		typedef basic_string<T, Safety>                         base_type;
		typedef typename base_type::allocator_type              allocator_type;
        typedef typename base_type::literal_type                literal_type;

		typedef typename base_type::value_type                  value_type;
		typedef typename base_type::pointer                     pointer;
		typedef typename base_type::const_pointer               const_pointer;
		typedef typename base_type::reference                   reference;
		typedef typename base_type::const_reference             const_reference;

		typedef typename base_type::iterator_safe               iterator;
		typedef typename base_type::const_iterator_safe         const_iterator;
		typedef typename base_type::reverse_iterator_safe       reverse_iterator;
		typedef typename base_type::const_reverse_iterator_safe const_reverse_iterator;

		typedef typename base_type::size_type                   size_type;
		typedef typename base_type::difference_type             difference_type;

        using base_type::npos;
		static constexpr memory_safety is_safe = Safety;

	public:
		// Constructor, destructor
		basic_string_safe() : base_type() {}
		basic_string_safe(const this_type& x, size_type position, size_type n = npos) : base_type(x, position, n) {}
		basic_string_safe(const value_type* p) : base_type(p) {}
		basic_string_safe(const literal_type& l) : base_type(l) {}
		basic_string_safe(size_type n, value_type c) : base_type(n, c) {}
		basic_string_safe(const this_type& x) = default;
		basic_string_safe(std::initializer_list<value_type> init) : base_type(init) {}
		basic_string_safe(this_type&& x) = default;

	   ~basic_string_safe() {}

		// Operator=
		this_type& operator=(const this_type& x) = default;
		this_type& operator=(const value_type* p) { base_type::operator=(p); return *this; }
		this_type& operator=(const literal_type& l) { base_type::operator=(l); return *this; }
		this_type& operator=(value_type c) { base_type::operator=(c); return *this; }
		this_type& operator=(std::initializer_list<value_type> ilist) { base_type::operator=(ilist); return *this; }
		this_type& operator=(this_type&& x) = default;

		// void swap(this_type& x) { base_type::swap(x); }

		// Assignment operations
		this_type& assign(const this_type& x) { base_type::assign(x); return *this; }
		this_type& assign(const this_type& x, size_type position, size_type n = npos) { base_type::assign(x, position, n); return *this; }
		this_type& assign(const value_type* p) { base_type::assign(p); return *this; }
		this_type& assign(const literal_type& l) { base_type::assign(l); return *this; }
		this_type& assign(size_type n, value_type c) { base_type::assign(n, c); return *this; }
		this_type& assign(this_type&& x) { base_type::assign(std::move(x)); return *this; }
		this_type& assign(std::initializer_list<value_type> init) { base_type::assign(init); return *this; }

		// Iterators.
		iterator       begin() noexcept { return base_type::begin_safe(); }
		const_iterator begin() const noexcept { return base_type::begin_safe(); }
		const_iterator cbegin() const noexcept { return base_type::cbegin_safe(); }

		iterator       end() noexcept { return base_type::end_safe(); }
		const_iterator end() const noexcept { return base_type::end_safe(); }
		const_iterator cend() const noexcept { return base_type::cend_safe(); }

		reverse_iterator       rbegin() noexcept { return base_type::rbegin_safe(); }
		const_reverse_iterator rbegin() const noexcept { return base_type::rbegin_safe(); }
		const_reverse_iterator crbegin() const noexcept { return base_type::crbegin_safe(); }

		reverse_iterator       rend() noexcept { return base_type::rend_safe(); }
		const_reverse_iterator rend() const noexcept { return base_type::rend_safe(); }
		const_reverse_iterator crend() const noexcept { return base_type::crend_safe(); }

		// Size-related functionality
        // using base_type::empty;
        // using base_type::size;
        // using base_type::length;
        // using base_type::max_size;
        // using base_type::capacity;
        // using base_type::resize;
        // using base_type::reserve;
        // using base_type::set_capacity;
        // using base_type::force_size; //TODO: review
        // using base_type::shrink_to_fit;

		// Raw access
        // using base_type::data;
        // using base_type::c_str;

		// Element access
		// reference       operator[](size_type n) {  return at(n); }
		// const_reference operator[](size_type n) const {  return at(n); }

		// reference       at(size_type n) {
        //     checkPos(n);
        //     return base_type::at(n);
        // }

		// const_reference at(size_type n) const {
        //     checkPos(n);
        //     return base_type::at(n);
        // }

        // using base_type::front;
        // using base_type::back;

		// Append operations
		this_type& operator+=(const this_type& x) { base_type::operator+=(x); return *this; }
		this_type& operator+=(const value_type* p) { base_type::operator+=(p); return *this; }
		this_type& operator+=(const literal_type& l) { base_type::operator+=(l); return *this; }
		this_type& operator+=(value_type c) { base_type::operator+=(c); return *this; }

		this_type& append(const this_type& x) { base_type::append(x); return *this; }
		this_type& append(const this_type& x,  size_type position, size_type n = npos) { base_type::append(x, position, n); return *this; }
		this_type& append(const value_type* p) { base_type::append(p); return *this; }
		this_type& append(const literal_type& l) { base_type::append(l); return *this; }
		this_type& append(size_type n, value_type c) { base_type::append(n, c); return *this; }

        // using base_type::push_back;

		// void pop_back() {
		// 	if constexpr (Safety == memory_safety::safe) {
		// 		if(NODECPP_UNLIKELY(empty())) {
		// 			ThrowRangeException("basic_string::pop_back -- empty string");
		// 		}
		// 	}
		// 	base_type::pop_back();
		// }

		// Insertion operations
		this_type& insert(size_type position, const this_type& x) { base_type::insert(position, x); return *this; }
		this_type& insert(size_type position, const this_type& x, size_type beg, size_type n) { base_type::insert(position, x, beg, n); return *this; }
		this_type& insert(size_type position, const value_type* p) { base_type::insert(position, p); return *this; }
		this_type& insert(size_type position, const literal_type& l) { base_type::insert(position, l); return *this; }
		this_type& insert(size_type position, size_type n, value_type c) { base_type::insert(position, n, c); return *this; }

		iterator   insert(const_iterator p, value_type c) { return base_type::insert_safe(p, c); }
		iterator   insert(const_iterator p, size_type n, value_type c) { return base_type::insert_safe(p, n, c); }
		iterator   insert(const_iterator p, std::initializer_list<value_type> init) { return base_type::insert_safe(p, init); }

		// Erase operations
		this_type&       erase(size_type position = 0, size_type n = npos) { base_type::erase(position, n); return *this; }

		iterator         erase(const_iterator p) { return base_type::erase_safe(p); }
        iterator         erase(const_iterator pBegin, const_iterator pEnd) { return base_type::erase_safe(pBegin, pEnd); }

        // using base_type::clear;

		// Replacement operations
		this_type&  replace(size_type position, size_type n,  const this_type& x) { base_type::replace(position, n, x); return *this; }
		this_type&  replace(size_type pos1,     size_type n1, const this_type& x,  size_type pos2, size_type n2 = npos) { base_type::replace(pos1, n1, x, pos2, n2); return *this; }
		this_type&  replace(size_type position, size_type n1, const value_type* p) { base_type::replace(position, n1, p); return *this; }
		this_type&  replace(size_type position, size_type n1, const literal_type& l) { base_type::replace(position, n1, l); return *this; }
		this_type&  replace(size_type position, size_type n1, size_type n2, value_type c) { base_type::replace(position, n1, n2, c); return *this; }

		this_type&  replace(const_iterator first, const_iterator last, const this_type& x) { base_type::replace_safe(first, last, x); return *this; }
		this_type&  replace(const_iterator first, const_iterator last, const literal_type& l) { base_type::replace_safe(first, last, l); return *this; }
		this_type&  replace(const_iterator first, const_iterator last, size_type n, value_type c) { base_type::replace_safe(first, last, n, c); return *this; }

	}; // basic_string_safe

} // namespace safememory


#endif //SAFE_MEMORY_STRING_H